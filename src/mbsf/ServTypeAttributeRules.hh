#ifndef __SERV_TYPE_ATTRIBUTE_RULES_HH__
#define __SERV_TYPE_ATTRIBUTE_RULES_HH__
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBS service type attribute rules
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

#include <list>
#include <string>
#include <utility>

#include "common.hh"

MBSF_NAMESPACE_START

/** Presence of the MBS Distribution Session attributes whose validity depends on the MBS
 * service type of the parent MBS User Service.
 *
 * Presence flags rather than the model object, so the rule can be exercised without building a
 * provisioning request.
 */
struct ServTypeAttributes {
    bool nrRedCapUeInfo = false;   //!< mbsDisSessInfos.<id>.nrRedCapUeInfo is present
    bool mbsFSAId = false;         //!< mbsDisSessInfos.<id>.mbsFSAId is present
    bool restrictedFlag = false;   //!< mbsDisSessInfos.<id>.restrictedFlag is present
    bool ssmMbsSessionId = false;  //!< mbsDisSessInfos.<id>.mbsSessionId carries an ssm
};

/** Every attribute @p serv_type does not permit, as attribute name and the reason.
 *
 * The names are relative to the Distribution Session, so a caller prefixes them with
 * "mbsDisSessInfos.<id>." before reporting them in invalidParams.
 *
 * The three attribute rules come from TS 29.580, which makes nrRedCapUeInfo and mbsFSAId
 * broadcast-only and restrictedFlag multicast-only.
 *
 * The identifier rule comes from TS 23.247 V18.8.0 clause 6.5.1, which gives the MBS Session ID
 * types as "-TMGI (for broadcast and multicast MBS sessions);" and "-source specific IP multicast
 * address (for multicast MBS sessions)", so an SSM identifies a multicast MBS Session only. A
 * broadcast session is identified by a TMGI, which the MBSF requests when mbsSessionId is absent.
 *
 * The two directions are deliberately not symmetric, and this is the behaviour the callers had
 * before this rule was named. An attribute permitted only for broadcast is refused whenever the
 * service type is anything else, including unknown, because an attribute that needs a broadcast
 * service cannot be honoured without one. The SSM is refused only when the service type is known
 * to be broadcast, because an absent or unrecognised service type does not establish that the
 * session is a broadcast one, and refusing on a guess would reject valid multicast provisioning.
 */
inline std::list<std::pair<std::string, std::string> >
servTypeViolations(const std::string &serv_type, const ServTypeAttributes &present)
{
    std::list<std::pair<std::string, std::string> > violations;

    if (serv_type != "BROADCAST") {
        if (present.nrRedCapUeInfo) {
            violations.emplace_back("nrRedCapUeInfo",
                                    "nrRedCapUeInfo is only applicable to a BROADCAST MBS User Service");
        }
        if (present.mbsFSAId) {
            violations.emplace_back("mbsFSAId",
                                    "mbsFSAId is only applicable to a BROADCAST MBS User Service");
        }
    }

    if (serv_type != "MULTICAST" && present.restrictedFlag) {
        violations.emplace_back("restrictedFlag",
                                "restrictedFlag is only applicable to a MULTICAST MBS User Service");
    }

    if (serv_type == "BROADCAST" && present.ssmMbsSessionId) {
        violations.emplace_back("mbsSessionId.ssm",
                                "an SSM identifies a multicast MBS Session; a BROADCAST MBS User Service is "
                                "identified by a TMGI, which the MBSF requests when mbsSessionId is absent");
    }

    return violations;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* __SERV_TYPE_ATTRIBUTE_RULES_HH__ */
