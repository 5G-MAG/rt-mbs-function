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

// Header include for this class
#include "AvailabilityInfo.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::AvailabilityInformation;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

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
    setMbsServiceAreas(service_areas);
    m_availabilityInformation->setMbsFSAId(mbs_fas_id);
}

AvailabilityInfo::AvailabilityInfo(const std::shared_ptr< ServiceArea > &service_area, const std::optional<std::string> &mbs_fas_id)
    :m_availabilityInformation(new AvailabilityInformation())
{
    m_serviceAreas.push_back(service_area);
    setMbsServiceAreas(m_serviceAreas);
    m_availabilityInformation->setMbsFSAId(mbs_fas_id);
}

AvailabilityInfo::AvailabilityInfo(const std::optional<std::shared_ptr<reftools::mbsf::MbsServiceArea>> &mbs_service_area, const std::optional<std::string> &mbs_fas_id)
    :m_availabilityInformation(new AvailabilityInformation())
{
    m_availabilityInformation->addMbsServiceAreas(mbs_service_area);
    m_availabilityInformation->setMbsFSAId(mbs_fas_id);
}


AvailabilityInfo::~AvailabilityInfo()
{
}

AvailabilityInfo &AvailabilityInfo::setMbsServiceAreas(const std::list<std::shared_ptr< ServiceArea > > &service_areas)
{
    for(const auto &service_area: service_areas) {
        if (!service_area) continue;
        std::shared_ptr<reftools::mbsf::MbsServiceArea>  mbs_service_area = service_area->getMbsServiceArea();
        if (!mbs_service_area) continue;
        m_availabilityInformation->addMbsServiceAreas(mbs_service_area);
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

