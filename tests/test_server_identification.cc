/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Server response header of the co-located MBS AF unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the “License”).  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */

/* Covers the Server response header of the co-located server (TS 26.517 V19.2.0 clauses 8.2.3.3.1, 8.2.3.3.2 and
 * 8.2.3.3.4): the product identifier names the type and the host name, separated by a single hyphen, with the release
 * number as the product version, the header reaches a response made by an HTTPServer, and a path wrapped for another
 * interface replaces it with that interface's own.
 */

#include <netinet/in.h>
#include <unistd.h>

#include <climits>
#include <cstdio>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include <HTTPRequest.hh>
#include <HTTPRequestHandler.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>
#include <SockAddr.hh>

#include "ServerIdentification.hh"

using namespace com::fiveg_mag::ref_tools::mbsf;
HTTPXPP_USING_NAMESPACE;

/* A handler that answers every request with the response its server makes, and counts shutdown calls. */
class StubHandler : public HTTPRequestHandler {
public:
    int shutdowns = 0;
    virtual void serverShutdown(const HTTPServer &) { shutdowns++; }
    virtual HTTPResponse doRequest(const HTTPRequest &, const HTTPServer &server) { return server.makeResponse().statusCode(204); }
};

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const std::string &what)
{
    if (ok) {
        g_pass++;
        std::printf("INFO: %s passed.\n", what.c_str());
    } else {
        g_fail++;
        std::printf("ERROR: %s failed.\n", what.c_str());
    }
}

int main(int argc, char *argv[])
{
    /* The example of clause 8.2.3.3.1, with the release as the version. */
    check(serverident::productIdentifier("MBSAF", "vm10240.mno.net") == "MBSAF-vm10240.mno.net/19",
          "type, one hyphen, host name and release number");
    check(serverident::productIdentifier("MBSAF", "my-host") == "MBSAF-my-host/19",
          "a hyphen inside the host name is kept; the first hyphen is the separator because the type has none");

    /* A host name is one product token whatever the machine reports. */
    check(serverident::productIdentifier("MBSAF", "a b/c") == "MBSAF-a-b-c/19",
          "a space or a slash in the host name is written as a hyphen");
    check(serverident::productIdentifier("MBSAF", "host_1") == "MBSAF-host_1/19", "an underscore is a token character");
    check(serverident::productIdentifier("MBSAF", "") == "MBSAF/19", "no host name is invented when none is known");

    check(serverident::headerValue("MBSAF", "h.example", "5G-MAG-HTTP-Server/1; microhttpd/2") ==
              "MBSAF-h.example/19 (5G-MAG-HTTP-Server/1; microhttpd/2)",
          "the product identifier is followed by the comment element");

    /* The host name of this machine is what gethostname() gives, and the identifier carries it. */
    char name[HOST_NAME_MAX + 1] = {0};
    const bool have_name = gethostname(name, sizeof(name) - 1) == 0 && name[0] != '\0';
    const std::string local = serverident::localHostName();
    check(have_name == !local.empty(), "the local host name is read exactly when gethostname() succeeds");
    if (have_name) {
        check(local == std::string(name), "the local host name is the one gethostname() reports");
        const std::string id = serverident::productIdentifier("MBSAF", local);
        check(id.starts_with("MBSAF-") && id.ends_with("/19") && id.find(' ') == std::string::npos,
              "the identifier built from the local host name is one token ending in the release");
    }

    /* The header is on a response made by the server it was set on. */
    SockAddr addr(sockaddr_in{.sin_family = AF_INET, .sin_port = 0, .sin_addr = {htonl(INADDR_LOOPBACK)}});
    auto handler = std::shared_ptr<HTTPRequestHandler>();
    HTTPServer server(addr, handler);
    const std::string value = serverident::headerValue("MBSAF", "h.example", "c");
    server.serverName(value);
    HTTPResponse response = server.makeResponse();
    check(response.getHeader("Server") == std::optional<std::string>(value), "a response made by the server carries the Server header");

    /* The MBS-1 path of the same server names itself MBSF, the MBS-5 path keeps the server's MBSAF. */
    auto stub = std::make_shared<StubHandler>();
    const std::string mbsf_value = serverident::headerValue("MBSF", "h.example", "c");
    serverident::ServerIdentifyingRequestHandler wrapped(stub, mbsf_value);
    HTTPRequest request("/x/", "GET", "HTTP/1.1", std::map<std::string, std::list<std::string> >{}, std::vector<char>{});
    HTTPResponse wrapped_response = wrapped.doRequest(request, server);
    check(wrapped_response.getHeader("Server") == std::optional<std::string>(mbsf_value),
          "a wrapped path answers with the Server header of its own interface, not one joined to the server's");
    check(wrapped_response.statusCode() == 204, "the wrapped handler's answer is otherwise unchanged");
    HTTPResponse plain_response = stub->doRequest(request, server);
    check(plain_response.getHeader("Server") == std::optional<std::string>(value),
          "a path that is not wrapped keeps the server's Server header");
    wrapped.serverShutdown(server);
    check(stub->shutdowns == 1, "a server shutdown reaches the wrapped handler");

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
