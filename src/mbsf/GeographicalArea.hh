#ifndef _MBSF_GEOGRAPHICAL_AREA_HH_
#define _MBSF_GEOGRAPHICAL_AREA_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS GeographicalArea class
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/GeographicArea.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class GeographicalCoordinates;
}

MBSF_NAMESPACE_START

class GeographicalArea {
public:

    GeographicalArea(fiveg_mag_reftools::CJson &json, bool as_request);
    GeographicalArea(const std::shared_ptr<reftools::mbsf::GeographicArea> &geographic_area);
    GeographicalArea() = delete;
    GeographicalArea(GeographicalArea &&other) = delete;
    GeographicalArea(const GeographicalArea &other) = delete;
    GeographicalArea &operator=(GeographicalArea &&other) = delete;
    GeographicalArea &operator=(const GeographicalArea &other) = delete;

    virtual ~GeographicalArea();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::GeographicArea> &getGeographicArea() const {return m_geographicArea;};
    const std::shared_ptr<reftools::mbsf::GeographicalCoordinates> &getPoint() const {return m_geographicArea->getPoint();};
    mb_smf_sc_geographic_area_t *populateGeographicArea();

private:
    std::shared_ptr<reftools::mbsf::GeographicArea> m_geographicArea;

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_GEOGRAPHICAL_AREA_HH_ */
