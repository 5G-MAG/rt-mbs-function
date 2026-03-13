#ifndef _MBSF_SERVICE_SCHEDULE_DESC_HH_
#define _MBSF_SERVICE_SCHEDULE_DESC_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Service Schedule Desc class
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

#include "openapi/model/ServiceScheduleDescription.h"
#include "common.hh"

MBSF_NAMESPACE_START

class ServiceScheduleDesc {
public:

    ServiceScheduleDesc(fiveg_mag_reftools::CJson &json, bool as_request);
    ServiceScheduleDesc(const std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> &service_schedule_description);
    ServiceScheduleDesc(const std::string &id, const int32_t version, const std::optional<std::string > &start = std::nullopt, const std::optional<std::string > &stop = std::nullopt);
    ServiceScheduleDesc(const std::string &id, const int32_t version, const std::optional<std::shared_ptr< reftools::mbsf::RepetitionRule > > &repetition_rule);
    ServiceScheduleDesc() = delete;
    ServiceScheduleDesc(ServiceScheduleDesc &&other) = delete;
    ServiceScheduleDesc(const ServiceScheduleDesc &other) = delete;
    ServiceScheduleDesc &operator=(ServiceScheduleDesc &&other) = delete;
    ServiceScheduleDesc &operator=(const ServiceScheduleDesc &other) = delete;

    virtual ~ServiceScheduleDesc();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> &changeServiceScheduleDescription(
    std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> new_service_schedule_description);

    const std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> &serviceScheduleDescription() const {return m_serviceScheduleDescription;};
    const std::optional<std::shared_ptr< reftools::mbsf::RepetitionRule > > &repetitionRule() const {return m_serviceScheduleDescription->getRepetitionRule();};
    const std::optional<std::string > &start() const {return m_serviceScheduleDescription->getStart();};
    const std::optional<std::string > &stop() const {return m_serviceScheduleDescription->getStop();};
    const int32_t version() const {return m_serviceScheduleDescription->getVersion();};

private:
    std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> m_serviceScheduleDescription;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_SERVICE_SCHEDULE_DESC_HH_ */
