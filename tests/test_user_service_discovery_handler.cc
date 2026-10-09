/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBS5 User Service Description retrieval handler unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the "License").  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */

/* Covers the HTTP details of the User Service Description retrieval API (TS 26.517 V18.6.0 clause
 * 9.2.2) that need no provisioned User Service: the answer to a method the API does not allow, and
 * the Cache-Control value of clause 8.2.3.4. The handler itself needs the MBSF application context,
 * which a unit test does not stand up; the rest of the API is exercised against a running MBSF.
 */

#include <netinet/in.h>

#include <cstdio>
#include <memory>
#include <optional>
#include <string>

#include <HTTPResponse.hh>
#include <HTTPServer.hh>
#include <SockAddr.hh>

#include "UserServiceDiscoveryHttp.hh"

using namespace com::fiveg_mag::ref_tools::mbsf;
HTTPXPP_USING_NAMESPACE;

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
    SockAddr addr(sockaddr_in{.sin_family = AF_INET, .sin_port = 0, .sin_addr = {htonl(INADDR_LOOPBACK)}});
    auto handler = std::shared_ptr<HTTPRequestHandler>();
    HTTPServer server(addr, handler);

    HTTPResponse response = discovery::methodNotAllowed(server);
    check(response.statusCode() == 405, "a method the API does not allow is answered 405");
    check(response.getHeader("Allow") == std::optional<std::string>("GET"), "the 405 names GET in Allow");

    check(discovery::cacheControlMaxAge(60) == "max-age=60", "max-age carries the period in seconds");
    check(discovery::cacheControlMaxAge(0) == "max-age=0", "a zero period is stated, not omitted");

    std::printf("Test: UserServiceDiscoveryHandler Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
