/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Service Area class
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
#include "MBSNcgi.hh"
#include "MBSNcgiTai.hh"
#include "TrackingAreaIdentity.hh"
#include "openapi/model/MbsServiceArea.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "ServiceArea.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::Ncgi;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::Tai;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

ServiceArea::ServiceArea(CJson &json, bool as_request)
    :m_mbsServiceArea(new MbsServiceArea(json, as_request))
{
}

ServiceArea::ServiceArea(const std::shared_ptr<MbsServiceArea> &mbs_service_area)
    :m_mbsServiceArea(mbs_service_area)
{
}

ServiceArea::~ServiceArea()
{
}

CJson ServiceArea::json(bool as_request = false) const
{
    return m_mbsServiceArea->toJSON(as_request);
}

mb_smf_sc_mbs_service_area_t *ServiceArea::populateServiceArea() {


    mb_smf_sc_mbs_service_area_t *mbs_service_area = mb_smf_sc_mbs_service_area_new();
    ncgiTais(mbs_service_area);
    tais(mbs_service_area);
    return mbs_service_area;
}

void ServiceArea::ncgiTais(mb_smf_sc_mbs_service_area_t *mbs_service_area) {

    //std::optional<std::list<std::optional<std::shared_ptr< NcgiTai > >
    const reftools::mbsf::MbsServiceArea::NcgiListType &ncgi_tais = getNcgiList();
    if (!ncgi_tais.has_value()) return;

    ogs_list_init(&mbs_service_area->ncgi_tais);

    for(const auto &ncgi_tai: ncgi_tais.value()) {
        if (ncgi_tai.has_value()) {
            std::shared_ptr< MBSNcgiTai > mbs_ncgi_tai = nullptr;
            std::shared_ptr< NcgiTai > recv_ncgi_tai = ncgi_tai.value();
            mbs_ncgi_tai.reset(new MBSNcgiTai(recv_ncgi_tai));
            mb_smf_sc_ncgi_tai_t *mb_smf_ncgi_tai = mbs_ncgi_tai->populateNcgiTai();
            ogs_list_add(&mbs_service_area->ncgi_tais, mb_smf_ncgi_tai);
        }

    }
}

void ServiceArea::tais(mb_smf_sc_mbs_service_area_t *mbs_service_area) {

    //std::optional<std::list<std::optional<std::shared_ptr< Tai > >
    const reftools::mbsf::MbsServiceArea::TaiListType &tais = getTaiList();
    if (!tais.has_value()) return;

    ogs_list_init(&mbs_service_area->tais);

    for(const auto &tai: tais.value()) {
        if (tai.has_value()) {
            std::shared_ptr< TrackingAreaIdentity > tracking_area_identity = nullptr;
            std::shared_ptr< Tai > recv_tai = tai.value();
            tracking_area_identity.reset(new TrackingAreaIdentity(recv_tai));
            mb_smf_sc_tai_t *mb_smf_tai = tracking_area_identity->populateTai();
            ogs_list_add(&mbs_service_area->tais, mb_smf_tai);
        }

    }
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

