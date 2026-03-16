/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Service Schedule Description class
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
#include "openapi/model/ServiceScheduleDescription.h"
//#include "openapi/model/RepetitionRule.h"

// Header include for this class
#include "ServiceScheduleDesc.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::ServiceScheduleDescription;

MBSF_NAMESPACE_START

ServiceScheduleDesc::ServiceScheduleDesc(CJson &json, bool as_request)
   :m_serviceScheduleDescription(new reftools::mbsf::ServiceScheduleDescription(json, as_request))
{
}

ServiceScheduleDesc::ServiceScheduleDesc(const std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> &service_schedule_description)
    :m_serviceScheduleDescription(service_schedule_description)
{
}

ServiceScheduleDesc::ServiceScheduleDesc(const std::string &id, const int32_t version, const std::optional<std::string > &start, const std::optional<std::string > &stop)
    :m_serviceScheduleDescription(new reftools::mbsf::ServiceScheduleDescription())
{

    m_serviceScheduleDescription->setId(id);
    m_serviceScheduleDescription->setStart(start);
    m_serviceScheduleDescription->setStop(stop);
    m_serviceScheduleDescription->setVersion(version);
}

ServiceScheduleDesc::ServiceScheduleDesc(const std::string &id, const int32_t version, const std::optional<std::shared_ptr< reftools::mbsf::RepetitionRule > > &repetition_rule)
    :m_serviceScheduleDescription(new reftools::mbsf::ServiceScheduleDescription())
{

    m_serviceScheduleDescription->setId(id);
    m_serviceScheduleDescription->setRepetitionRule(repetition_rule);
    m_serviceScheduleDescription->setVersion(version);

}

ServiceScheduleDesc::~ServiceScheduleDesc()
{
}

CJson ServiceScheduleDesc::json(bool as_request = false) const
{
    return m_serviceScheduleDescription->toJSON(as_request);
}

std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> &ServiceScheduleDesc::changeServiceScheduleDescription(
    std::shared_ptr<reftools::mbsf::ServiceScheduleDescription> new_service_schedule_description)
{

    m_serviceScheduleDescription->setStart(std::nullopt);
    m_serviceScheduleDescription->setStop(std::nullopt);
    m_serviceScheduleDescription->setRepetitionRule(std::nullopt);

    m_serviceScheduleDescription->setStart(new_service_schedule_description->getStart());
    m_serviceScheduleDescription->setStop(new_service_schedule_description->getStart());
    m_serviceScheduleDescription->setRepetitionRule(new_service_schedule_description->getRepetitionRule());
    //m_serviceScheduleDescription->setVersion(m_version);
    return m_serviceScheduleDescription;

}
MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

