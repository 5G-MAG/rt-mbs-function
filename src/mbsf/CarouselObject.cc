/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Carousel Object class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

#include "openapi/model/Object.h"

// Header include for this class
#include "CarouselObject.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::Object;

MBSF_NAMESPACE_START

CarouselObject::CarouselObject(CJson &json, bool as_request)
    :m_object(new Object(json, as_request))
{
}

CarouselObject::CarouselObject(const std::shared_ptr<reftools::mbsf::Object> &object)
    :m_object(object)
{
}

CarouselObject::CarouselObject(const std::string &locator, std::optional<int32_t > repetition_interval,
                    std::optional<int32_t > keep_updated_interval, std::optional<std::string > earliest_fetch_time)
{
    m_object.reset(new Object());
    m_object->setLocator(locator);
    m_object->setRepetitionInterval(repetition_interval);
    m_object->setKeepUpdatedInterval(keep_updated_interval);
    m_object->setEarliestFetchTime(earliest_fetch_time);
}


CarouselObject::~CarouselObject()
{
}

CJson CarouselObject::json(bool as_request = false) const
{
    return m_object->toJSON(as_request);
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

