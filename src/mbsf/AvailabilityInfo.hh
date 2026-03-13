#ifndef _MBSF_AVAILABILITY_INFO_HH_
#define _MBSF_AVAILABILITY_INFO_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Availability Information class
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

#include "mb-smf-service-consumer.h"

#include <memory>

#include "openapi/model/AvailabilityInformation.h"
#include "openapi/model/MbsServiceArea.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class ServiceArea;

class AvailabilityInfo {
public:

    AvailabilityInfo(fiveg_mag_reftools::CJson &json, bool as_request);
    AvailabilityInfo(const std::shared_ptr<reftools::mbsf::AvailabilityInformation> &availability_information);
    AvailabilityInfo(const std::list<std::shared_ptr< ServiceArea > > &service_areas, const std::optional<std::string> &mbs_fas_id = std::nullopt);
    AvailabilityInfo(const std::shared_ptr< ServiceArea > &service_area, const std::optional<std::string> &mbs_fas_id = std::nullopt);
    AvailabilityInfo(const std::optional<std::shared_ptr<reftools::mbsf::MbsServiceArea>> &mbs_service_area, const std::optional<std::string> &mbs_fas_id = std::nullopt);
    AvailabilityInfo() = delete;
    AvailabilityInfo(AvailabilityInfo &&other) = delete;
    AvailabilityInfo(const AvailabilityInfo &other) = delete;
    AvailabilityInfo &operator=(AvailabilityInfo &&other) = delete;
    AvailabilityInfo &operator=(const AvailabilityInfo &other) = delete;

    virtual ~AvailabilityInfo();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    AvailabilityInfo &addServiceArea(const std::shared_ptr< ServiceArea > &service_area);

    const std::shared_ptr<reftools::mbsf::AvailabilityInformation> &availabilityInformation() const {return m_availabilityInformation;};
    const reftools::mbsf::AvailabilityInformation::MbsServiceAreasType &mbsServiceAreas() const {return m_availabilityInformation->getMbsServiceAreas();};
    const std::optional<std::string > &mbsFSAId() const {return m_availabilityInformation->getMbsFSAId();};
    const reftools::mbsf::AvailabilityInformation::NrParametersType &nrParameters() const {return m_availabilityInformation->getNrParameters();};
    const reftools::mbsf::AvailabilityInformation::NrRedCapUEInfoType &nrRedCapUEInfo() const {return m_availabilityInformation->getNrRedCapUEInfo();};

private:
    AvailabilityInfo &setMbsServiceAreas(const std::list<std::shared_ptr< ServiceArea > > &service_areas);

    std::shared_ptr<reftools::mbsf::AvailabilityInformation> m_availabilityInformation;
    std::list<std::shared_ptr< ServiceArea > > m_serviceAreas;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_AVAILABILITY_INFO_HH_ */
