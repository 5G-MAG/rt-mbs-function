#ifndef _MBSF_CAROUSEL_OBJECT_HH_
#define _MBSF_CAROUSEL_OBJECT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Object Manifest class
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <chrono>
#include <memory>

#include "openapi/model/Object.h"

#include "common.hh"

namespace reftools::mbsf {
    class Object;
}

MBSF_NAMESPACE_START

class UserDataIngSession;

class CarouselObject {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    CarouselObject(fiveg_mag_reftools::CJson &json, bool as_request);
    CarouselObject(const std::shared_ptr<reftools::mbsf::Object> &object);
    CarouselObject(const std::string &locator, std::optional<int32_t > repetition_interval = std::nullopt,
                    std::optional<int32_t > keep_updated_interval = std::nullopt, std::optional<std::string > earliest_fetch_time = std::nullopt);

    CarouselObject() = delete;
    CarouselObject(CarouselObject &&other) = delete;
    CarouselObject(const CarouselObject &other) = delete;
    CarouselObject &operator=(CarouselObject &&other) = delete;
    CarouselObject &operator=(const CarouselObject &other) = delete;

    virtual ~CarouselObject();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::Object> object() const {return m_object;};

private:

    std::shared_ptr<reftools::mbsf::Object> m_object;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_CAROUSEL_OBJECT_HH_ */
