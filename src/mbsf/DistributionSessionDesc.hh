#ifndef _MBSF_DISTRIBUTION_SESSION_DESC_HH_
#define _MBSF_DISTRIBUTION_SESSION_DESC_HH_
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

#include "openapi/model/DistributionSessionDescription.h"
#include "common.hh"

MBSF_NAMESPACE_START

class ApplicationServiceDesc;
class AvailabilityInfo;
class ObjRepairParameters;
class ServiceArea;

class DistributionSessionDesc {
public:

    DistributionSessionDesc(fiveg_mag_reftools::CJson &json, bool as_request);
    DistributionSessionDesc(const std::shared_ptr<reftools::mbsf::DistributionSessionDescription> &description_session_description);
    DistributionSessionDesc(const std::shared_ptr<reftools::mbsf::DistributionMethod>& distribution_method,
                    const std::string& Session_description_locator,
                    std::optional<std::list<std::shared_ptr<ApplicationServiceDesc>>> application_service_descriptions = std::nullopt,
                    std::optional<std::shared_ptr<ObjRepairParameters>> object_repair_parameters = std::nullopt,
                    std::optional<std::list<std::shared_ptr<AvailabilityInfo>>> availabilty_infos = std::nullopt,
                    std::optional<std::list<std::string>> conformance_profiles = std::nullopt
    );

    DistributionSessionDesc(DistributionSessionDesc &&other) = delete;
    DistributionSessionDesc(const DistributionSessionDesc &other) = delete;
    DistributionSessionDesc &operator=(DistributionSessionDesc &&other) = delete;
    DistributionSessionDesc &operator=(const DistributionSessionDesc &other) = delete;

    virtual ~DistributionSessionDesc();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    DistributionSessionDesc &addApplicationServiceDesc(std::shared_ptr<ApplicationServiceDesc> &application_service_desc);
    DistributionSessionDesc &addApplicationServiceDesc(const std::string &entry_point_locator, const std::string &content_type);

    DistributionSessionDesc &setObjRepairParameters(std::shared_ptr<ObjRepairParameters> &obj_repair_parameters);

    DistributionSessionDesc &addAvailabilityInfo(std::shared_ptr<AvailabilityInfo> &availability_info);
    DistributionSessionDesc &addAvailabilityInfo(const std::list<std::shared_ptr< ServiceArea > > &service_areas, const std::optional<std::string> &mbs_fas_id = std::nullopt);
    DistributionSessionDesc &addAvailabilityInfo(const std::shared_ptr< ServiceArea > &service_areas, const std::optional<std::string> &mbs_fas_id = std::nullopt);

    DistributionSessionDesc &populateAndSetApplicationServiceDescriptions(const std::list<std::shared_ptr<ApplicationServiceDesc>> &application_service_descriptions);
    DistributionSessionDesc &populateAndSetApplicationInfos(const std::list<std::shared_ptr<AvailabilityInfo>> &availabilty_infos);
    DistributionSessionDesc &populateAndSetConformanceProfiles(std::optional<std::list<std::string>> conformance_profiles = std::nullopt);

    const std::shared_ptr<reftools::mbsf::DistributionSessionDescription> &distributionSessionDescription() const {return m_distributionSessionDescription;};
    const std::shared_ptr< reftools::mbsf::DistributionMethod > &distributionMethod() const {return m_distributionSessionDescription->getDistributionMethod();};
    const reftools::mbsf::DistributionSessionDescription::ConformanceProfilesType &conformanceProfile() const {return m_distributionSessionDescription->getConformanceProfiles();};
    const std::string &sessionDescriptionLocator() const {return m_distributionSessionDescription->getSessionDescriptionLocator();};
    const reftools::mbsf::DistributionSessionDescription::ApplicationServiceDescriptionsType &applicationServiceDescriptions() const
       {return m_distributionSessionDescription->getApplicationServiceDescriptions();};

    const reftools::mbsf::DistributionSessionDescription::PostSessionObjectRepairParametersType &objectRepairParameters() const {return m_distributionSessionDescription->getPostSessionObjectRepairParameters();};
    const reftools::mbsf::DistributionSessionDescription::AvailabilityInfosType &availabiltyInfos() const {return m_distributionSessionDescription->getAvailabilityInfos();};

private:

    std::shared_ptr<reftools::mbsf::DistributionSessionDescription> m_distributionSessionDescription;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_DISTRIBUTION_SESSION_DESC_HH_ */
