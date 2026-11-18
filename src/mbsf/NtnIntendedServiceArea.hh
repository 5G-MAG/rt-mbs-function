/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: NR NTN Intended Service Area derivation
 ******************************************************************************
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the "License").  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */
#ifndef _MBSF_NTN_INTENDED_SERVICE_AREA_HH_
#define _MBSF_NTN_INTENDED_SERVICE_AREA_HH_

#include <cstdint>
#include <optional>
#include <vector>

#include "common.hh"

/*
 * The NR NTN Intended Service Area derived from geographical area information (TS 23.247 V19.3.0 clause 6.21: "the NEF may determine
 * that an MBS session is targeting a NR NTN network and may be configured to include the NR NTN Intended Service Area derived from the
 * geographical area information in an MBS service area").
 *
 * The result is the octets of the Intended Service Area Coordinates IE of TS 38.413 clause 9.3.1.273, "the areaCoordinates contained
 * in the IntendedServiceAreaInfo IE as defined in TS 38.331": the unaligned PER encoding of areaCoordinates-r19, a CHOICE of
 * polygonArea-r19 (an OCTET STRING holding the Polygon of TS 37.355) and circleArea-r19 (the Ellipsoid-Point of TS 37.355 as an OCTET
 * STRING, and a radius in steps of 50 m). The two TS 37.355 types are ASN.1 types, so the longitude is the offset binary of its
 * INTEGER (-8388608..8388607) constraint. Reading, not confirmed by an independent implementation.
 */
MBSF_NAMESPACE_START

namespace NtnIntendedServiceArea {

struct Point {
    double lon;  /**< degrees, -180 to 180 */
    double lat;  /**< degrees, -90 to 90 */
};

/** polygonArea-r19: three to fifteen points (the Polygon of TS 37.355 allows no other count). */
std::optional<std::vector<uint8_t> > polygonCoordinates(const std::vector<Point> &points);

/** circleArea-r19: the centre and an uncertainty in metres, rounded up to the next step of 50 m, at most 65535 steps. */
std::optional<std::vector<uint8_t> > circleCoordinates(const Point &centre, double uncertainty_metres);

}

MBSF_NAMESPACE_STOP

#endif /* _MBSF_NTN_INTENDED_SERVICE_AREA_HH_ */
