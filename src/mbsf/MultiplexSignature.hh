#ifndef _MBSF_MULTIPLEX_SIGNATURE_HH_
#define _MBSF_MULTIPLEX_SIGNATURE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: what the members of a multiplex have to share
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 ******************************************************************************/
#include <memory>
#include <string>

#include "openapi/model/MBSDistributionSessionInfo.h"

/* Whether the Distribution Session is flagged as part of a multiplex. */
inline bool multiplexed(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info)
{
    const auto &flag = info->getMultiplexedServFlag();
    return flag.has_value() && flag.value();
}

/* What the MB-SMF is told about an MBS Session, as the text the members of a multiplex are compared on.
 *
 * TS 29.580 V18.8.0 table 6.2.6.2.4-1, multiplexedServFlag: the members have "identical or empty set(s) of target
 * service areas" and are "multiplexed onto the same MBS Session". An MBS Session is created with one set of target
 * service areas and one MBS Service Information (its QoS); the specification gives no rule for combining different ones.
 * So the members of a multiplex have to say the same about all of what the MB-SMF is told, and a member that says
 * something else keeps an MBS Session of its own. An absent value and an empty one are the same here. */
inline std::string multiplex_signature(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info)
{
    std::string signature;
    const auto &tgt = info->getTgtServAreas();
    if (tgt.has_value() && tgt.value()) signature += tgt.value()->toJSON(true).serialise();
    signature += '|';
    const auto &ext = info->getExtTgtServAreas();
    if (ext.has_value() && ext.value()) signature += ext.value()->toJSON(true).serialise();
    signature += '|';
    const auto &service_info = info->getMbsServInfo();
    if (service_info.has_value() && service_info.value()) signature += service_info.value()->toJSON(true).serialise();
    signature += '|';
    const auto &fsa_id = info->getMbsFSAId();
    if (fsa_id.has_value()) signature += fsa_id.value();
    signature += '|';
    const auto &location_dependent = info->getLocationDependent();
    signature += (location_dependent.has_value() && location_dependent.value()) ? "1" : "0";
    signature += '|';
    const auto &restricted = info->getRestrictedFlag();
    signature += (restricted.has_value() && restricted.value()) ? "1" : "0";
    return signature;
}

#endif /* _MBSF_MULTIPLEX_SIGNATURE_HH_ */
