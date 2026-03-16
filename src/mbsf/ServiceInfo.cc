/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Service Info class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// standard template library includes
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <optional>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "UserDataIngSession.hh"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/ObjectDistrMethInfo.h"

#include "MediaComp.hh"
#include "MediaInfo.hh"
#include "mb-smf-service-consumer.h"

// Header include for this class
#include "ServiceInfo.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsMediaCompRm;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::ReservPriority;

MBSF_NAMESPACE_START

ServiceInfo::ServiceInfo(CJson &json, bool as_request)
    :m_mbsServiceInfo(new MbsServiceInfo(json, as_request))
{
}

ServiceInfo::ServiceInfo(const std::shared_ptr<MbsServiceInfo> &mbs_service_info)
    :m_mbsServiceInfo(mbs_service_info)
{
}

ServiceInfo::~ServiceInfo()
{
}

CJson ServiceInfo::json(bool as_request = false) const
{
    return m_mbsServiceInfo->toJSON(as_request);
}

ogs_hash_t *ServiceInfo::populateMediaComps()
{

    const reftools::mbsf::MbsServiceInfo::MbsMediaCompsType &mbs_media_comps = m_mbsServiceInfo->getMbsMediaComps();
    ogs_hash_t *mb_smf_media_comps = ogs_hash_make();
    for(const auto &[key, mbs_media_comp]: mbs_media_comps) {
        if (mbs_media_comp.has_value()) {
            std::shared_ptr<MediaComp> media_comp = nullptr;

            std::shared_ptr<MbsMediaCompRm> comp = mbs_media_comp.value();

            media_comp.reset(new MediaComp(mbs_media_comp.value()));

            mb_smf_sc_mbs_media_comp_t *mb_smf_media_comp = media_comp->populateMediaComp();
            if (mb_smf_media_comp) {
                ogs_hash_set(mb_smf_media_comps, &mb_smf_media_comp->id, sizeof(mb_smf_media_comp->id), mb_smf_media_comp);
            }

        }

    }
    return mb_smf_media_comps;

}
/*
uint64_t *ServiceInfo::ambr() {
    const std::optional<std::string > &ambr = getMbsSessionAmbr();
    if (!ambr.has_value()) return nullptr;
    static uint64_t br;
    br = std::stoull(ambr.value());
    return &br;
}
*/

uint64_t *ServiceInfo::ambr() {
    const std::optional<std::string > &ambr = getMbsSessionAmbr();
    if (!ambr.has_value()) return nullptr;
    std::string br = ambr.value();

    // Trim leading/trailing spaces
    br.erase(0, br.find_first_not_of(" \t"));
    br.erase(br.find_last_not_of(" \t") + 1);

    // Extract number and unit
    std::stringstream ss(br);
    double value;
    std::string unit;
    ss >> value >> unit;

    // Normalize unit to lowercase
    std::transform(unit.begin(), unit.end(), unit.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    uint64_t multiplier = 1;

    if (unit == "bps") {
        multiplier = 1ULL;
    } else if (unit == "kbps") {
        multiplier = 1000ULL;
    } else if (unit == "mbps") {
        multiplier = 1000ULL * 1000ULL;
    } else if (unit == "gbps") {
        multiplier = 1000ULL * 1000ULL * 1000ULL;
    } else if (unit == "tbps") {
        multiplier = 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else {
        throw std::invalid_argument("Unknown unit: " + unit);
    }

    uint64_t *rv = new uint64_t(static_cast<uint64_t>(value * multiplier));
    return rv;
}


mb_smf_sc_mbs_service_info_t *ServiceInfo::populateServiceInfo()
{
    mb_smf_sc_mbs_service_info_t *service_info = mb_smf_sc_mbs_service_info_new();
    service_info->mbs_media_comps = populateMediaComps();
    const std::optional<std::string > &af_app_id = getAfAppId();
    if (af_app_id.has_value()) {
        service_info->af_app_id = ogs_strdup(af_app_id.value().c_str());
    } else {
        service_info->af_app_id = nullptr;
    }
    service_info->mbs_session_ambr = ambr();

    const std::optional<std::shared_ptr< ReservPriority > > &reserv_priority =  getSdfResPrio();
    if (reserv_priority.has_value()) {
        std::shared_ptr< ReservPriority > priority = reserv_priority.value();
        service_info->mbs_sdf_reserve_priority = static_cast<uint8_t>(priority->getValue());
    } else {
        service_info->mbs_sdf_reserve_priority = 0;
    }
    return service_info;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

