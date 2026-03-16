/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: External Service Area class
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
#include "CivicAddr.hh"
#include "GeographicalArea.hh"
#include "openapi/model/CivicAddress.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/GeographicArea.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "ExternalServiceArea.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::CivicAddress;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::GeographicArea;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

ExternalServiceArea::ExternalServiceArea(CJson &json, bool as_request)
    :m_externalMbsServiceArea(new ExternalMbsServiceArea(json, as_request))
{
}

ExternalServiceArea::ExternalServiceArea(const std::shared_ptr<ExternalMbsServiceArea> &external_mbs_service_area)
    :m_externalMbsServiceArea(external_mbs_service_area)
{
}

ExternalServiceArea::~ExternalServiceArea()
{
}

CJson ExternalServiceArea::json(bool as_request = false) const
{
    return m_externalMbsServiceArea->toJSON(as_request);
}

mb_smf_sc_ext_mbs_service_area_t *ExternalServiceArea::populateExternalServiceArea() {


    mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area = mb_smf_sc_ext_mbs_service_area_new();
    geographicAreas(ext_mbs_service_area);
    civicAddresses(ext_mbs_service_area);
    return ext_mbs_service_area;
}

void ExternalServiceArea::geographicAreas(mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area) {

    //std::optional<std::list<std::optional<std::shared_ptr< GeographicArea > >
    const reftools::mbsf::ExternalMbsServiceArea::GeographicAreaListType &geo_areas = getGeographicAreaList();
    if (!geo_areas.has_value()) return;

    ogs_list_init(&ext_mbs_service_area->geographic_areas);

    for(const auto &geo_area: geo_areas.value()) {
        if (geo_area.has_value()) {
            std::shared_ptr< GeographicalArea > geographical_area = nullptr;
            std::shared_ptr< GeographicArea > recv_geo_area = geo_area.value();
            geographical_area.reset(new GeographicalArea(recv_geo_area));

            mb_smf_sc_geographic_area_t *geographic_area = geographical_area->populateGeographicArea();
            ogs_list_add(&ext_mbs_service_area->geographic_areas, geographic_area);
        }

    }
}

void ExternalServiceArea::civicAddresses(mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area) {

    //std::optional<std::list<std::optional<std::shared_ptr< CivicAddress > >
    const reftools::mbsf::ExternalMbsServiceArea::CivicAddressListType &civic_addresses = getCivicAddressList();
    if (!civic_addresses.has_value()) return;

    ogs_list_init(&ext_mbs_service_area->civic_addresses);

    for(const auto &civic_address: civic_addresses.value()) {
        if (civic_address.has_value()) {
            std::shared_ptr< CivicAddr > civic_addr = nullptr;
            std::shared_ptr< CivicAddress > recv_civic_address = civic_address.value();
            civic_addr.reset(new CivicAddr(recv_civic_address));
            mb_smf_sc_civic_address_t *mb_smf_civic_address = civic_addr->populateCivicAddress();
            ogs_list_add(&ext_mbs_service_area->civic_addresses, mb_smf_civic_address);
        }

    }
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

