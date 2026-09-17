/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: PlmnId class
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

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "openapi/model/tai.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "MBSPlmnId.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::PlmnId;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

MBSPlmnId::MBSPlmnId(CJson &json, bool as_request)
    :m_plmnId(new PlmnId(json, as_request))
{
}

MBSPlmnId::MBSPlmnId(const std::shared_ptr<PlmnId> &plmn_id)
    :m_plmnId(plmn_id)
{
}

MBSPlmnId::~MBSPlmnId()
{
}

CJson MBSPlmnId::json(bool as_request = false) const
{
    return m_plmnId->toJSON(as_request);
}

uint16_t MBSPlmnId::mcc() {
    uint16_t uint16_mcc;
    unsigned long long tmp;

    const std::string &mcc = getMcc();

    tmp = std::stoull(mcc);

    tmp = std::clamp<unsigned long long>(tmp, 0ULL, static_cast<unsigned long long>(std::numeric_limits<uint16_t>::max()));

    //tmp = std::clamp(tmp, 0ULL, static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()));
    uint16_mcc = static_cast<uint16_t>(tmp);

    return uint16_mcc;
}

uint16_t MBSPlmnId::mnc() {
    uint16_t uint16_mnc;
    unsigned long long tmp;

    const std::string &mnc = getMnc();

    tmp = std::stoull(mnc);

    tmp = std::clamp<unsigned long long>(tmp, 0ULL, static_cast<unsigned long long>(std::numeric_limits<uint16_t>::max()));
    uint16_mnc = static_cast<uint16_t>(tmp);

    return uint16_mnc;

}



std::shared_ptr<PlmnId> MBSPlmnId::fromPlmnId(const ogs_plmn_id_t &plmn_id)
{
    // Mnc keeps its leading zeros: it is formatted to the digit count the PLMN Id records, not
    // to the width its numeric value happens to need, so a 3-digit MNC below 100 stays 3 digits.
    char mcc_str[8];
    char mnc_str[8];
    std::snprintf(mcc_str, sizeof(mcc_str), "%03u", ogs_plmn_id_mcc(&plmn_id));
    std::snprintf(mnc_str, sizeof(mnc_str), "%0*u", static_cast<int>(ogs_plmn_id_mnc_len(&plmn_id)),
                  ogs_plmn_id_mnc(&plmn_id));

    std::shared_ptr<PlmnId> result(new PlmnId());
    result->setMcc(std::string(mcc_str));
    result->setMnc(std::string(mnc_str));
    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

