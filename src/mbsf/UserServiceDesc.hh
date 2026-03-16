#ifndef _MBSF_USER_SERVICE_DESC_HH_
#define _MBSF_USER_SERVICE_DESC_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Distribution Session Description class
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

#include <memory>

#include "openapi/model/UserServiceDescription.h"
#include "common.hh"

MBSF_NAMESPACE_START

class DistributionSessionDesc;
class UserServiceDesc;
class ServiceScheduleDesc;

class UserServiceDesc {
public:

    using serviceNameLanguageDescription = std::pair<std::string, std::string>;

    UserServiceDesc(fiveg_mag_reftools::CJson &json, bool as_request);
    UserServiceDesc(const std::shared_ptr<reftools::mbsf::UserServiceDescription> &user_service_description);
    UserServiceDesc(const reftools::mbsf::UserServiceDescription::ServiceIdsType &service_ids,
                    const std::string &r_class, const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &names,
                    const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &descriptions,
                    const std::list<std::shared_ptr< DistributionSessionDesc > > &distribution_session_decs,
                    std::optional<std::list<std::shared_ptr< ServiceScheduleDesc > >> service_schedule_desc = std::nullopt,
                    std::optional<std::string > service_language = std::nullopt);
    UserServiceDesc() = delete;
    UserServiceDesc(UserServiceDesc &&other) = delete;
    UserServiceDesc(const UserServiceDesc &other) = delete;
    UserServiceDesc &operator=(UserServiceDesc &&other) = delete;
    UserServiceDesc &operator=(const UserServiceDesc &other) = delete;

    virtual ~UserServiceDesc();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    UserServiceDesc &addDistributionSessionDesc(std::shared_ptr<DistributionSessionDesc> &distribution_session_desc);
    UserServiceDesc &addServiceScheduleDesc(std::shared_ptr<ServiceScheduleDesc> &service_schedule_desc);

    UserServiceDesc &populateAndSetDistributionSessionDescriptions(const std::list<std::shared_ptr<DistributionSessionDesc>> &distribution_session_descs);
    UserServiceDesc &populateAndSetNames(const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &names);
    UserServiceDesc &populateAndSetDescriptions(const std::list<std::shared_ptr<UserServiceDesc::serviceNameLanguageDescription>> &descriptions);
    UserServiceDesc &populateAndSetServiceScheduleDescriptions(std::list<std::shared_ptr<ServiceScheduleDesc>> &service_schedule_descs);

    const std::shared_ptr<reftools::mbsf::UserServiceDescription> &userServiceDescription() const {return m_userServiceDescription;};
    const reftools::mbsf::UserServiceDescription::NamesType &names() const {return m_userServiceDescription->getNames();};
    const reftools::mbsf::UserServiceDescription::DescriptionsType &descriptions() const {return m_userServiceDescription->getDescriptions();};
    const std::optional<std::string> serviceLanguage() const {return m_userServiceDescription->getServiceLanguage();};

private:
    std::shared_ptr<reftools::mbsf::UserServiceDescription> m_userServiceDescription;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_SERVICE_DESC_HH_ */
