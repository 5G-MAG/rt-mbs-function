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

    // Use the length-aware constructor: mcc()/mnc() alone lose the MNC's actual
    // digit count (2 vs 3), which a plain numeric value under 100 cannot distinguish.
    return mb_smf_sc_tai_new_len(mcc, mnc, mbs_plmn_id->mncLen(), tracking_area, n_id);
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
    // TS 29.571 Nid is an 11-character hex string (44-bit SNPN Network Id) --
    // parse as base 16, matching the correct sibling implementation MBSNcgi::nid().
    uint64_t value = std::stoull(nid.value(), nullptr, 16);

    uint64_t *result = static_cast<uint64_t*>(std::malloc(sizeof(uint64_t)));
    if (result != nullptr) {
        *result = value;
    }
    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

