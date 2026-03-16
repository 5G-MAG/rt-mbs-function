/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Ncgi Tai class
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
#include "MBSNcgi.hh"
#include "TrackingAreaIdentity.hh"
#include "openapi/model/tai.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "MBSNcgiTai.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::Ncgi;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::Tai;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

MBSNcgiTai::MBSNcgiTai(CJson &json, bool as_request)
    :m_ncgiTai(new NcgiTai(json, as_request))
{
}

MBSNcgiTai::MBSNcgiTai(const std::shared_ptr<NcgiTai> &ncgi_tai)
    :m_ncgiTai(ncgi_tai)
{
}

MBSNcgiTai::~MBSNcgiTai()
{
}

CJson MBSNcgiTai::json(bool as_request = false) const
{
    return m_ncgiTai->toJSON(as_request);
}

mb_smf_sc_ncgi_tai_t *MBSNcgiTai::populateNcgiTai() {


    std::shared_ptr< TrackingAreaIdentity > tracking_area_id = nullptr;
    const std::shared_ptr< Tai > &tai = getTai();
    tracking_area_id.reset(new TrackingAreaIdentity(tai));
    mb_smf_sc_tai_t *mb_smf_tai = tracking_area_id->populateTai();
    mb_smf_sc_ncgi_tai_t *mb_smf_ncgi_tai = mb_smf_sc_ncgi_tai_new();
    mb_smf_ncgi_tai->tai = *mb_smf_tai;
    ncgis(mb_smf_ncgi_tai);
    return mb_smf_ncgi_tai;

}

void MBSNcgiTai::ncgis(mb_smf_sc_ncgi_tai_t *ncgi_tai) {

    //std::list<std::optional<std::shared_ptr< Ncgi > >
    const NcgiTai::CellListType &cells = getCellList();

    ogs_list_init(&ncgi_tai->ncgis);

    for(const auto &cell: cells) {
        if (cell.has_value()) {
            std::shared_ptr< MBSNcgi > mbs_ncgi = nullptr;
            std::shared_ptr< Ncgi > ncgi = cell.value();
            mbs_ncgi.reset(new MBSNcgi(ncgi));
            mb_smf_sc_ncgi_t *mb_smf_ncgi = mbs_ncgi->populateNcgi();
            ogs_list_add(&ncgi_tai->ncgis, mb_smf_ncgi);
        }

    }
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

