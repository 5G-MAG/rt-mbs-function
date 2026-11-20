#ifndef _MBS_F_ANNOUNCEMENT_MODES_HH_
#define _MBS_F_ANNOUNCEMENT_MODES_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: where the announcement of a User Service goes, by announcement mode
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

#include "common.hh"

MBSF_NAMESPACE_START

/** The announcement modes of a User Service (TS 29.580 V19.6.0, ServiceAnnouncementMode), as three flags.
 *
 * TS 26.517 V19.2.0 clause 8A.2: "When an MBS User Service is provisioned with an MBS User Service Announcement Mode of PASSED_BACK in the
 * servAnnModes array of the MBSUserService data type (see clause 6.1.6.2.2 of TS 29.580 [17]) the MBS User Service Announcement bundles of all
 * active MBS User Data Ingest Sessions are made available to the MBS Application Provider by the MBSF at reference point MBS-1."
 */
namespace announcementmodes {

struct Set {
    bool viaMbs5 = false;
    bool viaDistributionSession = false;
    bool passedBack = false;
};

/** Whether the announcement bundle of the service's Ingest Sessions has to be written: for each of the three modes. */
inline bool needsBundle(const Set &modes)
{
    return modes.viaMbs5 || modes.viaDistributionSession || modes.passedBack;
}

/** Whether the bundle is for the MBS Application Provider only, at MBS-1: such a service is not advertised on the MBS-4-MC announcement
 * channel (VIA_MBS_DISTRIBUTION_SESSION) and not at MBS-5 (VIA_MBS_5), which TS 29.580 V19.6.0 gives as the two other ways of advertising. */
inline bool passedBackOnly(const Set &modes)
{
    return modes.passedBack && !modes.viaMbs5 && !modes.viaDistributionSession;
}

} // namespace announcementmodes

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_F_ANNOUNCEMENT_MODES_HH_ */
