#ifndef __SERV_ANN_MODE_RULES_HH__
#define __SERV_ANN_MODE_RULES_HH__
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Service announcement mode rules
 ******************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Authors: Jordi J. Gimenez (5G-MAG)
 * Copyright: (C) 2026 5G-MAG Association
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://www.5g-mag.com/license
 */

#include "openapi/model/MBSUserService.h"
#include "openapi/model/ServiceAnnouncementMode.h"

#include "common.hh"

MBSF_NAMESPACE_START

/** The announcement paths that a set of Service announcement modes advertises an MBS User Service on.
 *
 * The two paths on which the MBSF itself serves the announcement. PASSED_BACK is not here: it hands
 * the announcement to the MBS Application Provider in the Ingest Session response and has its own
 * test, UserService::isServiceAnnModePassedBack().
 */
struct ServAnnPaths {
    bool mbs5 = false;                 //!< VIA_MBS_5: the User Service Description retrieval API of the MBS AF
    bool distributionSession = false;  //!< VIA_MBS_DISTRIBUTION_SESSION: the User Service Announcement Channel
};

/** Which paths @p modes selects.
 *
 * TS 26.502 V19.6.0 clause 4.5.3, table 4.5.3-1, "Service announcement modes" (1..*): "Determines
 * whether the MBS User Service Announcement compiled by the MBSF is advertised to the MBSF Client at
 * reference point MBS-5 and/or advertised to the MBSF Client via the MBS Distribution Session and/or
 * passed back to the MBS Application Provider via reference point Nmb10."
 *
 * Any combination may be provisioned, so each mode is read on its own: a path is selected when its
 * mode is among @p modes, and not otherwise.
 */
inline ServAnnPaths servAnnPaths(const reftools::mbsf::MBSUserService::ServAnnModesType &modes)
{
    ServAnnPaths paths;
    for (const auto &mode : modes) {
        if (!mode.has_value() || !mode.value()) continue;
        switch (mode.value()->getValue()) {
        case reftools::mbsf::ServiceAnnouncementMode::VAL_VIA_MBS_5:
            paths.mbs5 = true;
            break;
        case reftools::mbsf::ServiceAnnouncementMode::VAL_VIA_MBS_DISTRIBUTION_SESSION:
            paths.distributionSession = true;
            break;
        default:
            break;
        }
    }
    return paths;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* __SERV_ANN_MODE_RULES_HH__ */
