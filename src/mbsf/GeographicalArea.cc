/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: GeographicalArea class
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
#include "GeographicalCoordinate.hh"
#include "openapi/model/GeographicArea.h"
#include "openapi/model/SupportedGADShapes.h"
#include "openapi/model/UncertaintyEllipse.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "GeographicalArea.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::GeographicalCoordinates;
using reftools::mbsf::GeographicArea;
using reftools::mbsf::SupportedGADShapes;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

GeographicalArea::GeographicalArea(CJson &json, bool as_request)
    :m_geographicArea(new GeographicArea(json, as_request))
{
}

GeographicalArea::GeographicalArea(const std::shared_ptr<GeographicArea> &geographic_area)
    :m_geographicArea(geographic_area)
{
}

GeographicalArea::~GeographicalArea()
{
}

CJson GeographicalArea::json(bool as_request = false) const
{
    return m_geographicArea->toJSON(as_request);
}

mb_smf_sc_geographic_area_t *GeographicalArea::populateGeographicArea()
{
    /* TS 29.572 GeographicArea is an anyOf of seven shapes; the shape member names the one in use and the model
       has already checked that its own attributes are present. */
    const GeographicArea &area = *m_geographicArea;
    if (!area.getShape()) return nullptr;

    auto lon = [](const std::shared_ptr<GeographicalCoordinates> &c) { return c ? c->getLon() : 0.0; };
    auto lat = [](const std::shared_ptr<GeographicalCoordinates> &c) { return c ? c->getLat() : 0.0; };
    const auto &point = area.getPoint();
    const auto &ellipse = area.getUncertaintyEllipse();

    switch (area.getShape()->getValue()) {
    case SupportedGADShapes::VAL_POINT:
        return mb_smf_sc_ga_point_new(lon(point), lat(point));
    case SupportedGADShapes::VAL_POINT_UNCERTAINTY_CIRCLE:
        return mb_smf_sc_ga_point_uncertainty_circle_new(lon(point), lat(point), area.getUncertainty());
    case SupportedGADShapes::VAL_POINT_UNCERTAINTY_ELLIPSE:
        if (!ellipse) return nullptr;
        return mb_smf_sc_ga_point_uncertainty_ellipse_new(lon(point), lat(point), ellipse->getSemiMajor(),
                ellipse->getSemiMinor(), ellipse->getOrientationMajor(), area.getConfidence());
    case SupportedGADShapes::VAL_POLYGON: {
        mb_smf_sc_geographic_area_t *polygon = mb_smf_sc_ga_polygon_new();
        for (const auto &item : area.getPointList()) {
            if (item && item.value()) mb_smf_sc_ga_polygon_add_point(polygon, lon(item.value()), lat(item.value()));
        }
        return polygon;
    }
    case SupportedGADShapes::VAL_POINT_ALTITUDE:
        return mb_smf_sc_ga_point_altitude_new(lon(point), lat(point), area.getAltitude());
    case SupportedGADShapes::VAL_POINT_ALTITUDE_UNCERTAINTY: {
        if (!ellipse) return nullptr;
        const auto &v_confidence = area.getVConfidence();
        uint8_t v_conf = v_confidence ? static_cast<uint8_t>(*v_confidence) : 0;
        return mb_smf_sc_ga_point_altitude_uncertainty_new(lon(point), lat(point), area.getAltitude(),
                ellipse->getSemiMajor(), ellipse->getSemiMinor(), ellipse->getOrientationMajor(),
                area.getUncertaintyAltitude(), area.getConfidence(), v_confidence ? &v_conf : nullptr);
    }
    case SupportedGADShapes::VAL_ELLIPSOID_ARC:
        return mb_smf_sc_ga_ellipsoid_arc_new(lon(point), lat(point), area.getInnerRadius(), area.getUncertaintyRadius(),
                area.getOffsetAngle(), area.getIncludedAngle(), area.getConfidence());
    default:
        return nullptr;
    }
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

