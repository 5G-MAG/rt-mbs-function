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

/** Read a configured max-age period, in seconds.
 *
 * RFC 9111 section 1.2.2: "A recipient parsing a delta-seconds value and converting it to binary form ought to use
 * an arithmetic type of at least 31 bits of non-negative integer range." A value outside 0 to 2147483647, or one
 * that is not all digits, is refused, so that "-1" is not read as 4294967295 seconds and a larger value is not
 * silently cut to what fits.
 *
 * \param text    the configured value.
 * \param seconds set to the value when it is accepted.
 * \return false when the value is refused, in which case `seconds` is left as it was.
 */
inline bool parseMaxAgeSeconds(const std::string &text, unsigned int &seconds)
{
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) return false;
    if (text.size() > 10) return false;
    const unsigned long long value = std::stoull(text);
    if (value > 2147483647ULL) return false;
    seconds = static_cast<unsigned int>(value);
    return true;
}

} // namespace discovery

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_F_USER_SERVICE_DISCOVERY_HTTP_HH_ */
