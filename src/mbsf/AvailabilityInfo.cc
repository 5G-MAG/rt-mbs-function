/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Availability Information class
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
#include "ServiceArea.hh"
#include "openapi/model/AvailabilityInformation.h"
#include "openapi/model/Ncgi.h"
#include "openapi/model/NcgiTai.h"
#include "openapi/model/Tai.h"
#include "openapi/model/TargetServiceArea.h"

// Header include for this class
#include "AvailabilityInfo.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::AvailabilityInformation;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

/* The Target service areas of an MBS Service Area as the announcement states them.
 *
 * TS 26.517 V19.2.0 table 5.2.9-1a: "Exactly one of the following properties shall be present: ncgiList, taiList,
 * geographicAreaList." A target service area is therefore a list of NR cell identifiers or a list of tracking area
 * identifiers, and an MBS Service Area that has both gives two target service areas, which together are the same
 * region. The MBS Service Area groups the cells of a tracking area (NcgiTai); the announcement lists the cells. The
 * Intended Service Areas of an MBS Service Area are not among the three alternatives and are not announced. */
static std::list<std::shared_ptr<reftools::mbsf::TargetServiceArea> >
targetServiceAreasOf(const std::shared_ptr<reftools::mbsf::MbsServiceArea> &mbs_service_area)
{
    std::list<std::shared_ptr<reftools::mbsf::TargetServiceArea> > result;
    if (!mbs_service_area) return result;

    std::shared_ptr<reftools::mbsf::TargetServiceArea> cells(new reftools::mbsf::TargetServiceArea());
    bool have_cells = false;
    const auto &ncgi_tais = mbs_service_area->getNcgiList();
    if (ncgi_tais.has_value()) {
        for (const auto &ncgi_tai : ncgi_tais.value()) {
            if (!ncgi_tai.has_value() || !ncgi_tai.value()) continue;
            for (const auto &ncgi : ncgi_tai.value()->getCellList()) {
                if (!ncgi.has_value() || !ncgi.value()) continue;
                cells->addNcgiList(ncgi);
                have_cells = true;
            }
        }
    }
    if (have_cells) result.push_back(cells);

    std::shared_ptr<reftools::mbsf::TargetServiceArea> tais(new reftools::mbsf::TargetServiceArea());
    bool have_tais = false;
    const auto &tai_list = mbs_service_area->getTaiList();
    if (tai_list.has_value()) {
        for (const auto &tai : tai_list.value()) {
            if (!tai.has_value() || !tai.value()) continue;
            tais->addTaiList(tai);
            have_tais = true;
        }
    }
    if (have_tais) result.push_back(tais);

    return result;
}

AvailabilityInfo::AvailabilityInfo(CJson &json, bool as_request)
    :m_availabilityInformation(new AvailabilityInformation(json, as_request))
{
}

AvailabilityInfo::AvailabilityInfo(const std::shared_ptr<reftools::mbsf::AvailabilityInformation> &availability_information)
    :m_availabilityInformation(availability_information)
{
}

AvailabilityInfo::AvailabilityInfo(const std::list<std::shared_ptr< ServiceArea > > &service_areas, const std::optional<std::string> &mbs_fas_id)
    :m_availabilityInformation(new AvailabilityInformation())
    ,m_serviceAreas(service_areas)
{
    setTargetServiceAreas(service_areas);
    m_availabilityInformation->setMbsFSAId(mbs_fas_id);
}

AvailabilityInfo::AvailabilityInfo(const std::shared_ptr< ServiceArea > &service_area, const std::optional<std::string> &mbs_fas_id)
    :m_availabilityInformation(new AvailabilityInformation())
{
    m_serviceAreas.push_back(service_area);
    setTargetServiceAreas(m_serviceAreas);
    m_availabilityInformation->setMbsFSAId(mbs_fas_id);
}

AvailabilityInfo::AvailabilityInfo(const std::optional<std::shared_ptr<reftools::mbsf::MbsServiceArea>> &mbs_service_area, const std::optional<std::string> &mbs_fas_id)
    :m_availabilityInformation(new AvailabilityInformation())
{
    if (mbs_service_area.has_value()) {
        for (const auto &target_service_area : targetServiceAreasOf(mbs_service_area.value())) {
            m_availabilityInformation->addTargetServiceAreas(target_service_area);
        }
    }
    m_availabilityInformation->setMbsFSAId(mbs_fas_id);
}


AvailabilityInfo::~AvailabilityInfo()
{
}

AvailabilityInfo &AvailabilityInfo::addNrParameters(int32_t freq_band_indicator, int32_t arfcn_value)
{
    std::shared_ptr<reftools::mbsf::NrParameterSet> nr_parameter_set(new reftools::mbsf::NrParameterSet());
    nr_parameter_set->setFreqBandIndicator(freq_band_indicator);
    nr_parameter_set->setARFCNValue(arfcn_value);
    m_availabilityInformation->addNrParameters(nr_parameter_set);
    return *this;
}

AvailabilityInfo &AvailabilityInfo::setTargetServiceAreas(const std::list<std::shared_ptr< ServiceArea > > &service_areas)
{
    for(const auto &service_area: service_areas) {
        if (!service_area) continue;
        for (const auto &target_service_area : targetServiceAreasOf(service_area->getMbsServiceArea())) {
            m_availabilityInformation->addTargetServiceAreas(target_service_area);
        }
    }
    return *this;
}

CJson AvailabilityInfo::json(bool as_request = false) const
{
    return m_availabilityInformation->toJSON(as_request);
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

