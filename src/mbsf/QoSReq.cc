/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: QoS Req class
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
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cctype>


// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "AllocationRetentionPriority.hh"
#include "openapi/model/Arp.h"
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
#include "openapi/model/MbsQoSReq.h"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/ObjectDistrMethInfo.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "QoSReq.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::Arp;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MbsQoSReq;

MBSF_NAMESPACE_START

QoSReq::QoSReq(CJson &json, bool as_request)
    :m_mbsQoSReq(new MbsQoSReq(json, as_request))
{
}

QoSReq::QoSReq(const std::shared_ptr<MbsQoSReq> &mbs_qos_req)
    :m_mbsQoSReq(mbs_qos_req)
{
}

QoSReq::~QoSReq()
{
}

CJson QoSReq::json(bool as_request = false) const
{
    return m_mbsQoSReq->toJSON(as_request);
}

mb_smf_sc_mbs_qos_req_t *QoSReq::populateQoSReq() {

    std::shared_ptr<AllocationRetentionPriority> alloc_retention_priority = nullptr;
    mb_smf_sc_mbs_qos_req_t *qos_req = mb_smf_sc_mbs_qos_req_new();
    qos_req->five_qi = getQosReqR5qi();
    qos_req->guarenteed_bit_rate = bitrate(getGuardBitRate());
    qos_req->max_bit_rate = bitrate(getMaxBitRate());
    qos_req->averaging_window = averagingWindow();
    const std::optional<std::shared_ptr< Arp > > &arp = getReqMbsArp();
    if(arp.has_value()) {
        alloc_retention_priority.reset(new AllocationRetentionPriority(arp.value()));
        qos_req->req_mbs_arp =  alloc_retention_priority->populateArp();
    } else {
        qos_req->req_mbs_arp = nullptr;
    }
    return qos_req;
}

uint64_t *QoSReq::bitrate(const std::optional<std::string > &bit_rate) {

    if (!bit_rate.has_value()) return nullptr;
    std::string br = bit_rate.value();

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

    uint64_t *brate = new uint64_t(static_cast<uint64_t>(value * multiplier));
    return brate;
}

uint16_t *QoSReq::averagingWindow() {
    std::optional<int32_t> win = getAverageWindow();
    if (!win.has_value()) return nullptr;

    int32_t clamped = std::clamp(win.value(), int32_t(0), int32_t(UINT16_MAX));
    uint16_t *window = new uint16_t(static_cast<uint16_t>(clamped));
    return window;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

