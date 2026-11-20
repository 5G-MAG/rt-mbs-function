#ifndef _MBS_F_ANNOUNCEMENT_BUNDLE_URL_HH_
#define _MBS_F_ANNOUNCEMENT_BUNDLE_URL_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: URLs of the announcement bundle of an Ingest Session
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "common.hh"

MBSF_NAMESPACE_START

/** The two URLs under which the co-located server exposes the bundle of one Ingest Session.
 *
 * The same files are served on two paths because two reference points use them and the two name their server differently
 * (TS 26.517 V19.2.0 clause 8.2.3.3.2: "The product identifier token shall be set to the value MBSF." at MBS-1, and clause
 * 8.2.3.3.4: "The product identifier token shall be set to the value MBSAF." at MBS-11):
 * - the object locator that the MBSTF pulls for the announcement carousel, served by the MBS AF;
 * - the URL passed back to the MBS Application Provider, served by the MBSF. Clause 8A.2: "The format of URLs used to
 *   expose MBS User Service Announcement bundle at this reference point is left to implementation."
 */
namespace bundleurl {

/** Path prefix of the object locator (MBS-11). Not changed: it is the path the MBSF has always announced to the MBSTF. */
inline const std::string &objectLocatorPath()
{
    static const std::string path("/x-5gmag-service-announcements/v1/user-data-ingest-session/");
    return path;
}

/** Path prefix of the bundle at MBS-1. */
inline const std::string &mbs1Path()
{
    static const std::string path("/x-5gmag-service-announcements/v1/mbs-1/user-data-ingest-session/");
    return path;
}

/** The object locator of an Ingest Session's bundle, as the MBSTF is given it. */
inline std::string objectLocator(const std::string &authority, const std::string &ingest_session_id)
{
    return std::format("http://{}{}{}", authority, objectLocatorPath(), ingest_session_id);
}

/** The MBS-1 URL of an Ingest Session's bundle, from the authority of the co-located server, as a directory URL. Same string as
 * mbs1UrlFromObjectLocator() gives for objectLocator() of the same arguments. */
inline std::string mbs1Url(const std::string &authority, const std::string &ingest_session_id)
{
    return std::format("http://{}{}{}/", authority, mbs1Path(), ingest_session_id);
}

/** The MBS-1 URL of the bundle that an object locator names, or nothing when the locator is not one of ours. */
inline std::optional<std::string> mbs1UrlFromObjectLocator(std::string_view locator)
{
    const std::string_view marker(objectLocatorPath());
    const auto pos = locator.find(marker);
    if (pos == std::string_view::npos || !locator.starts_with("http://")) return std::nullopt;
    const std::string_view id = locator.substr(pos + marker.size());
    if (id.empty() || id.find('/') != std::string_view::npos) return std::nullopt;
    return std::format("{}{}{}/", locator.substr(0, pos), mbs1Path(), id);
}

} // namespace bundleurl

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_F_ANNOUNCEMENT_BUNDLE_URL_HH_ */
