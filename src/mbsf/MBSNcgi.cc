/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Ncgi class
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
#include "MBSPlmnId.hh"
#include "openapi/model/tai.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "MBSNcgi.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::Ncgi;
using reftools::mbsf::PlmnId;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

MBSNcgi::MBSNcgi(CJson &json, bool as_request)
    :m_ncgi(new Ncgi(json, as_request))
{
}

MBSNcgi::MBSNcgi(const std::shared_ptr<Ncgi> &ncgi)
    :m_ncgi(ncgi)
{
}

MBSNcgi::~MBSNcgi()
{
}

CJson MBSNcgi::json(bool as_request = false) const
{
    return m_ncgi->toJSON(as_request);
}

mb_smf_sc_ncgi_t *MBSNcgi::populateNcgi() {
    
    std::shared_ptr< MBSPlmnId > mbs_plmn_id = nullptr;
    uint16_t mcc;
    uint16_t mnc;

    const std::shared_ptr< PlmnId > &plmn_id = getPlmnId();
    mbs_plmn_id.reset(new MBSPlmnId(plmn_id));
    mcc = mbs_plmn_id->mcc();
    mnc = mbs_plmn_id->mnc();
    
    mb_smf_sc_ncgi_t *ncgi = mb_smf_sc_ncgi_new();

    mb_smf_sc_ncgi_set_plmn_id(ncgi, mcc, mnc);
    uint64_t cell_id = nrCellId();
    ncgi->nr_cell_id = static_cast<uint64_t>(cell_id) & ((1ULL << 36) - 1);
    ncgi->nid = nid();
    return ncgi;
    
}

uint64_t MBSNcgi::nrCellId() {
    const std::string &nr_cell_id = getNrCellId();
    uint64_t cell_id = std::stoull(nr_cell_id, nullptr, 16);
    return cell_id;
}

uint64_t *MBSNcgi::nid() {
    const std::optional<std::string > &nid = getNid();
    if(!nid.has_value()) return nullptr;
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

