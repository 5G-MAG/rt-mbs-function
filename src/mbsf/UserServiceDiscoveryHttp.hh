#ifndef _MBS_F_USER_SERVICE_DISCOVERY_HTTP_HH_
#define _MBS_F_USER_SERVICE_DISCOVERY_HTTP_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: HTTP details of the MBS5 User Service Description retrieval API
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <format>
#include <string>

#include <HTTPResponse.hh>
#include <HTTPServer.hh>

#include "common.hh"

MBSF_NAMESPACE_START

namespace discovery {

/** The answer to a method the API does not allow.
 *
 * TS 26.517 V18.6.0 table 9.2.2-1 lists GET as the only allowed method of both operations. RFC 9110
 * clause 10.2.1: "An origin server MUST generate an Allow header field in a 405 (Method Not Allowed)
 * response".
 */
inline HTTPXPP_NAMESPACE_NAME(HTTPResponse) methodNotAllowed(const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server)
{
    HTTPXPP_NAMESPACE_NAME(HTTPResponse) response = server.makeResponse();
    response.addHeader("Allow", "GET");
    response.statusCode(405);
    return response;
}

/** The Cache-Control field value of a 200 or 304 answer.
 *
 * TS 26.517 V18.6.0 clause 8.2.3.4: "a predicted time-to-live period for the resource, conveyed in a
 * Cache-Control: max-age response header".
 */
inline std::string cacheControlMaxAge(unsigned int seconds)
{
    return std::format("max-age={}", seconds);
}

} // namespace discovery

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_F_USER_SERVICE_DISCOVERY_HTTP_HH_ */
