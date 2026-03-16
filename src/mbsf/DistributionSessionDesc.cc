/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Distribution Session Description class
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
#include "ApplicationServiceDesc.hh"
#include "AvailabilityInfo.hh"
#include "ObjRepairParameters.hh"
#include "openapi/model/DistributionSessionDescription.h"

// Header include for this class
#include "DistributionSessionDesc.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistributionSessionDescription;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

DistributionSessionDesc::DistributionSessionDesc(CJson &json, bool as_request)
    :m_distributionSessionDescription(new DistributionSessionDescription(json, as_request))

{
}

DistributionSessionDesc::DistributionSessionDesc(const std::shared_ptr<reftools::mbsf::DistributionSessionDescription> &distribution_session_description)
    :m_distributionSessionDescription(distribution_session_description)
{
}

DistributionSessionDesc::DistributionSessionDesc(const std::shared_ptr<reftools::mbsf::DistributionMethod>& distribution_method,
                    const std::string& Session_description_locator,
                    std::optional<std::list<std::shared_ptr<ApplicationServiceDesc>>> application_service_descriptions,
                    std::optional<std::shared_ptr<ObjRepairParameters>> object_repair_parameters,
                    std::optional<std::list<std::shared_ptr<AvailabilityInfo>>> availabilty_infos,
                    std::optional<std::list<std::string>> conformance_profiles)
    :m_distributionSessionDescription(new DistributionSessionDescription())

{
    m_distributionSessionDescription->setDistributionMethod(distribution_method);
    m_distributionSessionDescription->setSessionDescriptionLocator(Session_description_locator);
    if (application_service_descriptions.has_value() && !application_service_descriptions->empty()) {
        populateAndSetApplicationServiceDescriptions(application_service_descriptions.value());
    }
    if (object_repair_parameters.has_value()) {
        m_distributionSessionDescription->setPostSessionObjectRepairParameters(object_repair_parameters.value()->objectRepairParameters());
    }
    if (availabilty_infos.has_value() && !availabilty_infos->empty()) {
        populateAndSetApplicationInfos(availabilty_infos.value());

    }
    if (conformance_profiles.has_value() && !conformance_profiles->empty()) {
        populateAndSetConformanceProfiles(conformance_profiles);
    }

}

DistributionSessionDesc::~DistributionSessionDesc()
{
}

CJson DistributionSessionDesc::json(bool as_request = false) const
{
    return m_distributionSessionDescription->toJSON(as_request);
}

DistributionSessionDesc &DistributionSessionDesc::populateAndSetApplicationServiceDescriptions(const std::list<std::shared_ptr<ApplicationServiceDesc>> &application_service_descriptions)
{
    for(const auto &application_service_description: application_service_descriptions) {
        if (!application_service_description) continue;
        const std::shared_ptr<reftools::mbsf::ApplicationServiceDescription> &application_service_desc = application_service_description->applicationServiceDescription();
        if (!application_service_desc) continue;
        m_distributionSessionDescription->addApplicationServiceDescriptions(application_service_desc);
    }
    return *this;

}

DistributionSessionDesc &DistributionSessionDesc::populateAndSetApplicationInfos(const std::list<std::shared_ptr<AvailabilityInfo>> &availability_infos)
{
    for(const auto &availability_info : availability_infos) {
        if (!availability_info) continue;
        const std::shared_ptr<reftools::mbsf::AvailabilityInformation> &avail_info = availability_info->availabilityInformation();
        if (!avail_info) continue;
        m_distributionSessionDescription->addAvailabilityInfos(avail_info);
    }
    return *this;

}

DistributionSessionDesc &DistributionSessionDesc::populateAndSetConformanceProfiles(std::optional<std::list<std::string>> conformance_profiles)
{
    if (!conformance_profiles.has_value()) {
        m_distributionSessionDescription->addConformanceProfiles("urn:3GPP:26517:17:baseline");
    } else {
        const std::list<std::string> &profiles = conformance_profiles.value();
        for(const auto &profile : profiles) {
            m_distributionSessionDescription->addConformanceProfiles(profile);
        }
    }
    return *this;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

