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
#include <cmath>
#include <cstddef>

#include "NtnIntendedServiceArea.hh"

MBSF_NAMESPACE_START

namespace NtnIntendedServiceArea {

namespace {

class BitWriter {
public:
    void put(uint64_t value, unsigned bits)
    {
        for (unsigned i = bits; i > 0; --i) {
            const unsigned bit = static_cast<unsigned>((value >> (i - 1)) & 1u);
            if (m_used % 8 == 0) m_octets.push_back(0);
            if (bit) m_octets.back() |= static_cast<uint8_t>(0x80u >> (m_used % 8));
            ++m_used;
        }
    }
    std::vector<uint8_t> octets() const { return m_octets; }   /* the last octet is padded with zero bits */
private:
    std::vector<uint8_t> m_octets;
    size_t m_used = 0;
};

/* The Ellipsoid-Point of TS 37.355: latitudeSign (north 0, south 1), degreesLatitude INTEGER (0..8388607) and degreesLongitude
 * INTEGER (-8388608..8388607), the point coding of TS 23.032: N <= 2^23 / 90 * X < N + 1 for the latitude and
 * N <= 2^24 / 360 * X < N + 1 for the longitude. 48 bits, written as the UPER encoding of the SEQUENCE: 1, 23 and 24 bits. */
bool putPoint(BitWriter &w, const Point &p)
{
    if (!(p.lat >= -90.0 && p.lat <= 90.0) || !(p.lon >= -180.0 && p.lon <= 180.0)) return false;   /* also false for NaN */
    int64_t lat = static_cast<int64_t>(std::floor(std::fabs(p.lat) * 8388608.0 / 90.0));
    if (lat > 8388607) lat = 8388607;                                   /* 90 degrees */
    int64_t lon = static_cast<int64_t>(std::floor(p.lon * 16777216.0 / 360.0));
    if (lon > 8388607) lon = 8388607;                                   /* 180 degrees */
    if (lon < -8388608) lon = -8388608;
    w.put(p.lat < 0.0 ? 1 : 0, 1);
    w.put(static_cast<uint64_t>(lat), 23);
    w.put(static_cast<uint64_t>(lon + 8388608), 24);                    /* offset binary of the constrained INTEGER */
    return true;
}

}

std::optional<std::vector<uint8_t> > polygonCoordinates(const std::vector<Point> &points)
{
    if (points.size() < 3 || points.size() > 15) return std::nullopt;

    BitWriter polygon;                                                  /* the Polygon: SEQUENCE (SIZE (3..15)) OF PolygonPoints */
    polygon.put(points.size() - 3, 4);
    for (const Point &p : points) {
        if (!putPoint(polygon, p)) return std::nullopt;
    }
    const std::vector<uint8_t> octets = polygon.octets();               /* at most 91 octets: a one octet length determinant */

    BitWriter area;                                                     /* areaCoordinates-r19, alternative polygonArea-r19 */
    area.put(0, 1);
    area.put(octets.size(), 8);
    for (uint8_t octet : octets) area.put(octet, 8);
    return area.octets();
}

std::optional<std::vector<uint8_t> > circleCoordinates(const Point &centre, double uncertainty_metres)
{
    if (!(uncertainty_metres >= 0.0)) return std::nullopt;              /* also NaN */
    const double steps = std::ceil(uncertainty_metres / 50.0);
    if (steps > 65535.0) return std::nullopt;

    BitWriter centre_point;                                             /* center-r19: the OCTET STRING of an Ellipsoid-Point */
    if (!putPoint(centre_point, centre)) return std::nullopt;
    const std::vector<uint8_t> octets = centre_point.octets();          /* 6 octets */

    BitWriter area;                                                     /* areaCoordinates-r19, alternative circleArea-r19 */
    area.put(1, 1);
    area.put(octets.size(), 8);
    for (uint8_t octet : octets) area.put(octet, 8);
    area.put(static_cast<uint64_t>(steps), 16);                         /* radius-r19 INTEGER (0..65535) */
    return area.octets();
}

}

MBSF_NAMESPACE_STOP
