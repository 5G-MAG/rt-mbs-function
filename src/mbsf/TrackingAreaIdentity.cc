/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: TrackingAreaIdentity class
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
#include <cctype>
#include <cstdlib>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "MBSPlmnId.hh"
#include "openapi/model/tai.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "TrackingAreaIdentity.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::Tai;
using reftools::mbsf::PlmnId;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

TrackingAreaIdentity::TrackingAreaIdentity(CJson &json, bool as_request)
    :m_tai(new Tai(json, as_request))
{
}

TrackingAreaIdentity::TrackingAreaIdentity(const std::shared_ptr<Tai> &tai)
    :m_tai(tai)
{
}

TrackingAreaIdentity::~TrackingAreaIdentity()
{
}

CJson TrackingAreaIdentity::json(bool as_request = false) const
{
    return m_tai->toJSON(as_request);
}

mb_smf_sc_tai_t *TrackingAreaIdentity::populateTai() {

    std::shared_ptr< MBSPlmnId > mbs_plmn_id = nullptr;
    uint16_t mcc;
    uint16_t mnc;
    uint32_t tracking_area;
    uint64_t *n_id = nullptr;


    const std::shared_ptr< PlmnId > &plmn_id = getPlmnId();
    mbs_plmn_id.reset(new MBSPlmnId(plmn_id));
    mcc = mbs_plmn_id->mcc();
    mnc = mbs_plmn_id->mnc();
    tracking_area = tac();
    n_id = nid();

    return mb_smf_sc_tai_new(mcc, mnc, tracking_area, n_id);
}

uint32_t TrackingAreaIdentity::tac() {
    const std::string &tac = getTac();
    return static_cast<uint32_t>(std::stoul(tac, nullptr, 16) & 0xFFFFFF);
}


/*
uint32_t TrackingAreaIdentity::tac() {
    bool hasHexLetter = false;
    const std::string &tac = getTac();
    for (char ch : tac) {
        if ((ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
            hasHexLetter = true;
            break;
        }
    }

    if (hasHexLetter) {
        // interpret as hex
        return static_cast<uint32_t>(std::stoul(tac, nullptr, 16) & 0xFFFFFF);
    } else {
        // interpret as decimal
        return static_cast<uint32_t>(std::stoul(tac, nullptr, 10) & 0xFFFFFF);
    }
}
*/






uint64_t* TrackingAreaIdentity::nid() {
    const std::optional<std::string > &nid = getNid();
    if (!nid.has_value()) return nullptr;
    uint64_t value = 0;
    for (char ch : nid.value()) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            value = value * 10 + (ch - '0');
        }
    }

    uint64_t *result = static_cast<uint64_t*>(std::malloc(sizeof(uint64_t)));
    if (result != nullptr) {
        *result = value;
    }
    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

