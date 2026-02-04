#ifndef _MBSF_GEOGRAPHICAL_COORDINATE_HH_
#define _MBSF_GEOGRAPHICAL_COORDINATE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS GeographicalCoordinate class
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

#include "mb-smf-service-consumer.h"

#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/GeographicalCoordinates.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class GeographicalCoordinate {
public:

    GeographicalCoordinate(fiveg_mag_reftools::CJson &json, bool as_request);
    GeographicalCoordinate(const std::shared_ptr<reftools::mbsf::GeographicalCoordinates> &geographical_coordinates);
    GeographicalCoordinate() = delete;
    GeographicalCoordinate(GeographicalCoordinate &&other) = delete;
    GeographicalCoordinate(const GeographicalCoordinate &other) = delete;
    GeographicalCoordinate &operator=(GeographicalCoordinate &&other) = delete;
    GeographicalCoordinate &operator=(const GeographicalCoordinate &other) = delete;

    virtual ~GeographicalCoordinate();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::GeographicalCoordinates> &getGeographicalCoordinates() const {return m_geographicalCoordinates;};
    const double &getLon() const {return m_geographicalCoordinates->getLon();};
    const double &getLat() const {return m_geographicalCoordinates->getLat();};

    mb_smf_sc_geographic_coordinate_t *populateGeographicCoordinate();

private:
    std::shared_ptr<reftools::mbsf::GeographicalCoordinates> m_geographicalCoordinates;

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_GEOGRAPHICAL_COORDINATE_HH_ */
