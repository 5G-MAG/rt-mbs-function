#ifndef _MBS_F_OBJECT_REPAIR_LOCATOR_HH_
#define _MBS_F_OBJECT_REPAIR_LOCATOR_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: the object repair base locator of a Distribution Session
 ******************************************************************************
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <optional>
#include <string>

#include "common.hh"

MBSF_NAMESPACE_START

/** The value of objectRepairBaseLocator announced for one Distribution Session.
 *
 * TS 26.502 V19.5.0 table 4.5.6-2: the Object repair base URL is "A URL prefix substituted by the MBSTF Client in place of the Object distribution base
 * URL when repairing objects not received completely intact from this MBS Distribution Session". The substitution is textual, so what follows the
 * distribution base URL in an object's location follows the repair base URL in the request: a repair base URL has to end with a slash exactly when the
 * distribution base URL does, or the two parts of the request URL run together ("...{id}obj.bin") or are separated twice.
 *
 * The MBS AS decides what path under its address holds a session's objects; the MBSF only has to announce the one that the MBS AS uses.
 * - An MBS AS that is not the MBSTF's keeps the scheme the MBSF has always announced: "{base}/{ingest session id}/{distribution session key}".
 * - The MBS AS co-located with the MBSTF (TS 26.502 V19.5.0 clause 4.3.4) finds a session's objects by the identifier of the Distribution Session in
 *   the MBSTF, which the MBSF chose and sent in the Nmb2 request: "{base}/{MBSTF distribution session id}". With no such identifier nothing can be
 *   announced, and a locator that points at nothing is not given.
 *
 * \param object_distribution_base_locator  the distribution base URL announced with it, when there is one.
 */
inline std::optional<std::string> announcedRepairBaseLocator(const std::string &base, bool hosted_by_mbstf, const std::string &user_data_ing_session_id,
                                                             const std::string &distribution_session_info_key, const std::string &mbstf_distribution_session_id,
                                                             const std::optional<std::string> &object_distribution_base_locator = std::nullopt)
{
    std::string trimmed(base);
    if (!trimmed.empty() && trimmed.back() == '/') trimmed.pop_back();
    const std::string end = (object_distribution_base_locator && !object_distribution_base_locator->empty() && object_distribution_base_locator->back() == '/') ? "/" : "";
    if (hosted_by_mbstf) {
        if (mbstf_distribution_session_id.empty()) return std::nullopt;
        return trimmed + "/" + mbstf_distribution_session_id + end;
    }
    return trimmed + "/" + user_data_ing_session_id + "/" + distribution_session_info_key + end;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_F_OBJECT_REPAIR_LOCATOR_HH_ */
