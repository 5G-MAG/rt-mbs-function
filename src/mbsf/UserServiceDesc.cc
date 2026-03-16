/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: User Service Description class
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
#include "DistributionSessionDesc.hh"
#include "ServiceScheduleDesc.hh"
#include "openapi/model/UserServiceDescription.h"
#include "openapi/model/UserServiceDescription_descriptions_inner.h"
#include "openapi/model/UserServiceDescription_names_inner.h"

// Header include for this class
#include "UserServiceDesc.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::UserServiceDescription;
using reftools::mbsf::UserServiceDescription_descriptions_inner;
using reftools::mbsf::UserServiceDescription_names_inner;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

UserServiceDesc::UserServiceDesc(CJson &json, bool as_request)
    :m_userServiceDescription(new UserServiceDescription(json, as_request))
{
}

UserServiceDesc::UserServiceDesc(const std::shared_ptr<reftools::mbsf::UserServiceDescription> &user_service_description)
    :m_userServiceDescription(user_service_description)
{
}

UserServiceDesc::UserServiceDesc(const reftools::mbsf::UserServiceDescription::ServiceIdsType &service_ids,
                    const std::string &r_class, const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &names,
                    const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &descriptions,
                    const std::list<std::shared_ptr< DistributionSessionDesc > > &distribution_session_descs,
                    std::optional<std::list<std::shared_ptr< ServiceScheduleDesc > >> service_schedule_descs,
                    std::optional<std::string > service_language)
    :m_userServiceDescription(new UserServiceDescription())
{
    m_userServiceDescription->setServiceIds(service_ids);
    m_userServiceDescription->setRClass(r_class);
    populateAndSetDistributionSessionDescriptions(distribution_session_descs);
    if (service_schedule_descs.has_value()) {
        populateAndSetServiceScheduleDescriptions(service_schedule_descs.value());
    }
    populateAndSetNames(names);
    populateAndSetDescriptions(descriptions);
    m_userServiceDescription->setServiceLanguage(service_language);
}

UserServiceDesc::~UserServiceDesc()
{
}

CJson UserServiceDesc::json(bool as_request = false) const
{
    return m_userServiceDescription->toJSON(as_request);
}

UserServiceDesc &UserServiceDesc::populateAndSetDistributionSessionDescriptions(const std::list<std::shared_ptr<DistributionSessionDesc>> &distribution_session_descs)
{
    for(const auto &distribution_session_desc : distribution_session_descs) {
        if (!distribution_session_desc) continue;
        const std::shared_ptr<reftools::mbsf::DistributionSessionDescription> &distribution_session_description = distribution_session_desc->distributionSessionDescription();
        if (!distribution_session_description) continue;
        m_userServiceDescription->addDistributionSessionDescriptions(distribution_session_description);
    }
    return *this;

}

UserServiceDesc &UserServiceDesc::populateAndSetServiceScheduleDescriptions(std::list<std::shared_ptr<ServiceScheduleDesc>> &service_schedule_descs)
{
    for(const auto &service_schedule_desc : service_schedule_descs) {
        if (!service_schedule_desc) continue;
        const std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> &service_schedule_description = service_schedule_desc->serviceScheduleDescription();
        if (!service_schedule_description) continue;
        m_userServiceDescription->addServiceScheduleDescriptions(service_schedule_description);
    }
    return *this;

}

UserServiceDesc &UserServiceDesc::populateAndSetNames(const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &names)
{
     for(const auto &name: names) {
        std::shared_ptr<reftools::mbsf::UserServiceDescription_names_inner> user_service_description_names = nullptr;
        if (!name) continue;

        user_service_description_names.reset(new UserServiceDescription_names_inner());
        user_service_description_names->setName(name->first);
        user_service_description_names->setLang(name->second);

        m_userServiceDescription->addNames(std::move(user_service_description_names));
    }
    return *this;
}

UserServiceDesc &UserServiceDesc::populateAndSetDescriptions(const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &descriptions)
{
     for(const auto &description: descriptions) {
        std::shared_ptr<reftools::mbsf::UserServiceDescription_descriptions_inner> user_service_description_descriptions = nullptr;
        if (!description) continue;

        user_service_description_descriptions.reset(new UserServiceDescription_descriptions_inner());
        user_service_description_descriptions->setDescription(description->first);
        user_service_description_descriptions->setLang(description->second);

        m_userServiceDescription->addDescriptions(std::move(user_service_description_descriptions));
    }
    return *this;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

