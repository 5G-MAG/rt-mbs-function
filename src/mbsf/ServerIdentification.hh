#ifndef _MBS_F_SERVER_IDENTIFICATION_HH_
#define _MBS_F_SERVER_IDENTIFICATION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Server response header of the co-located server
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <unistd.h>

#include <climits>
#include <format>
#include <memory>
#include <string>

#include <HTTPRequest.hh>
#include <HTTPRequestHandler.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>

#include "common.hh"

MBSF_NAMESPACE_START

namespace serverident {

/** The 3GPP release number the server announces in its product identifier.
 *
 * TS 26.517 V19.2.0 clause 8.2.3.3.1: "The optional product-version suffix shall be present and should indicate the
 * version number of the present document (without the leading "V") with which the server implementation complies and
 * shall, at minimum, indicate the 3GPP release number with which the implementation complies."
 */
inline const char *releaseNumber() { return "19"; }

/** A character that may appear in a product token of a Server header field value (the "tchar" set of RFC 9110). */
inline bool isTokenChar(unsigned char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    switch (c) {
    case '!': case '#': case '$': case '%': case '&': case '\'': case '*': case '+': case '-':
    case '.': case '^': case '_': case '`': case '|': case '~':
        return true;
    default:
        return false;
    }
}

/** The product identifier: server type, a single hyphen, host name, and the release number as the product version.
 *
 * TS 26.517 V19.2.0 clause 8.2.3.3.1: "The server type and host name shall be separated by a single hyphen ('-')
 * character." EXAMPLE 2 of that clause is "MBSAF-vm10240.mno.net/17". A character of the host name that cannot be part of
 * a token is written as a hyphen, so that the value is still one product token. An empty host name leaves the type and
 * the version only: no host name is invented.
 *
 * \param type the product identifier token of the function: "MBSAF" for the MBS AF (clause 8.2.3.3.4) and "MBSF" for the
 *             MBSF at MBS-1 (clause 8.2.3.3.2).
 * \param host the host name of the responding server.
 */
inline std::string productIdentifier(const std::string &type, const std::string &host)
{
    std::string token(type);
    if (!host.empty()) {
        token += '-';
        for (unsigned char c : host) token += isTokenChar(c) ? static_cast<char>(c) : '-';
    }
    return std::format("{}/{}", token, releaseNumber());
}

/** The value of the Server response header field: the product identifier followed by a comment element.
 *
 * TS 26.517 V19.2.0 clause 8.2.3.3.1: "The Server response header field may also include comment elements ... following
 * the above specified product identifier".
 */
inline std::string headerValue(const std::string &type, const std::string &host, const std::string &comment)
{
    return std::format("{} ({})", productIdentifier(type, host), comment);
}

/** The host name of this machine as gethostname() reports it, or an empty string when it cannot be read. */
inline std::string localHostName()
{
    char name[HOST_NAME_MAX + 1] = {0};
    if (gethostname(name, sizeof(name) - 1) != 0) return std::string();
    return std::string(name);
}

/** A request handler that sets the Server response header of every response of the handler it wraps.
 *
 * One HTTP server carries two interfaces: the MBS-1 bundle path of the MBSF and the MBS-5 API of the MBS AF. The two
 * name themselves differently (TS 26.517 V19.2.0 clause 8.2.3.3.2: "The product identifier token shall be set to the
 * value MBSF." and clause 8.2.3.3.4: "The product identifier token shall be set to the value MBSAF."), so each path is
 * wrapped with the header of its own interface.
 */
class ServerIdentifyingRequestHandler : public HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler) {
public:
    ServerIdentifyingRequestHandler(const std::shared_ptr<HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler)> &delegate,
                                    const std::string &server_header)
        :HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler)(), m_delegate(delegate), m_serverHeader(server_header) {}

    virtual void serverShutdown(const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server) { m_delegate->serverShutdown(server); }

    virtual HTTPXPP_NAMESPACE_NAME(HTTPResponse) doRequest(const HTTPXPP_NAMESPACE_NAME(HTTPRequest) &request,
                                                           const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server)
    {
        HTTPXPP_NAMESPACE_NAME(HTTPResponse) response = m_delegate->doRequest(request, server);
        response.setHeader("Server", m_serverHeader);
        return response;
    }

private:
    std::shared_ptr<HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler)> m_delegate;
    std::string m_serverHeader;
};

} // namespace serverident

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_F_SERVER_IDENTIFICATION_HH_ */
