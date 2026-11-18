/* The NR NTN Intended Service Area that the MBSF derives from geographical areas (TS 23.247 V19.3.0 clause 6.21): the octets of the
 * Intended Service Area Coordinates IE of TS 38.413 clause 9.3.1.273, the unaligned PER encoding of the areaCoordinates-r19 CHOICE of
 * TS 38.331 (polygonArea-r19, circleArea-r19) with the Polygon and Ellipsoid-Point of TS 37.355.
 *
 * The expected octets were computed on paper and by a second, separate program from the field widths of those definitions (a CHOICE
 * bit, a one octet length, 4 bits of point count, 1 + 23 + 24 bits per point with the longitude as the offset binary of its INTEGER
 * range, 16 bits of radius); they are not the output of another implementation of the specification. */
#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ExternalServiceArea.hh"
#include "NtnIntendedServiceArea.hh"
#include "openapi/model/CJson.hh"

using namespace com::fiveg_mag::ref_tools::mbsf;
using namespace com::fiveg_mag::ref_tools::mbsf::NtnIntendedServiceArea;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

static std::string hex(const std::vector<uint8_t> &octets)
{
    static const char digits[] = "0123456789abcdef";
    std::string s;
    for (uint8_t o : octets) { s += digits[o >> 4]; s += digits[o & 15]; }
    return s;
}

static std::string polygon(const std::vector<Point> &points)
{
    auto c = polygonCoordinates(points);
    return c ? hex(*c) : "none";
}

static std::string circle(double lon, double lat, double metres)
{
    auto c = circleCoordinates(Point{lon, lat}, metres);
    return c ? hex(*c) : "none";
}

int main()
{
    /* a polygon: 1 + 8 bits, 4 bits of count, 3 points of 48 bits, padded: 21 octets */
    check(polygon({{12.5, 41.9}, {13.0, 41.9}, {12.75, 42.5}}) == "0981dcba9c471c71dcba9c49f499e38e3c48888800",
          "a triangle in Rome gives the octets worked out by hand");
    /* a circle: 1 + 8 bits, one Ellipsoid-Point of 48 bits, 16 bits of radius: 10 octets */
    check(circle(12.5, 41.9, 120.0) == "831dcba9c471c7000180", "a circle of 120 m (3 steps of 50 m) in Rome");
    check(circle(-70.25, -33.5, 50.0) == "8357d27d2705b0000080", "a circle in the south west: the sign bit is set, the longitude is negative");
    check(circle(0.0, 0.0, 0.0) == "83000000400000000000", "the point 0, 0 with no uncertainty: the offset binary of longitude 0 is 2^23");

    /* the ends of the ranges */
    check(circle(180.0, 90.0, 0.0) == "833fffffffffff800000", "180 degrees east and 90 degrees north are the largest codes");
    check(circle(-180.0, -90.0, 0.0) == "837fffff800000000000", "180 degrees west and 90 degrees south: the sign and the smallest longitude");

    /* the radius is rounded up to a step of 50 m */
    check(circle(0.0, 0.0, 49.9) == "83000000400000000080", "49.9 m is one step");
    check(circle(0.0, 0.0, 50.0) == "83000000400000000080", "50 m is one step");
    check(circle(0.0, 0.0, 50.1) == "83000000400000000100", "50.1 m is two steps");
    check(circle(0.0, 0.0, 65535.0 * 50.0) == "830000004000007fff80", "65535 steps is the largest radius");
    check(circle(0.0, 0.0, 65535.0 * 50.0 + 0.1) == "none", "a radius above 65535 steps is refused, not cut");

    /* what is refused */
    check(polygon({{0, 0}, {1, 1}}) == "none", "a polygon of two points is refused");
    {
        std::vector<Point> sixteen;
        for (int i = 0; i < 16; i++) sixteen.push_back(Point{double(i), double(i)});
        check(polygon(sixteen) == "none", "a polygon of sixteen points is refused, the Polygon of TS 37.355 stops at fifteen");
        sixteen.pop_back();
        check(polygon(sixteen) != "none", "a polygon of fifteen points is accepted");
    }
    check(polygon({{0, 0}, {1, 1}, {0, 91}}) == "none", "a latitude of 91 degrees is refused");
    check(polygon({{0, 0}, {1, 1}, {181, 0}}) == "none", "a longitude of 181 degrees is refused");
    check(circle(NAN, 0.0, 10.0) == "none", "a longitude that is not a number is refused");
    check(circle(0.0, 0.0, -1.0) == "none", "a negative uncertainty is refused");

    /* the areas of an external MBS service area: one each for a polygon and a circle, none for another shape (a polygon of fewer than three
       points does not get as far, the model refuses it) */
    {
        const char *text = R"({"geographicAreaList":[
            {"shape":"POLYGON","pointList":[{"lon":12.5,"lat":41.9},{"lon":13.0,"lat":41.9},{"lon":12.75,"lat":42.5}]},
            {"shape":"POINT","point":{"lon":12.5,"lat":41.9}},
            {"shape":"POINT_UNCERTAINTY_CIRCLE","point":{"lon":12.5,"lat":41.9},"uncertainty":120.0}
        ]})";
        fiveg_mag_reftools::CJson json = fiveg_mag_reftools::CJson::parse(text);
        ExternalServiceArea area(json, true);
        const auto derived = area.deriveIntendedServiceAreas();
        check(derived.size() == 2, "of three geographic areas the polygon and the circle give one area each, the point none");
        if (derived.size() == 2) {
            check(hex(derived[0]) == "0981dcba9c471c71dcba9c49f499e38e3c48888800", "the first derived area is the polygon, in order");
            check(hex(derived[1]) == "831dcba9c471c7000180", "the second derived area is the circle");
        }
    }
    {
        fiveg_mag_reftools::CJson json = fiveg_mag_reftools::CJson::parse(R"({"civicAddressList":[{"country":"IT"}]})");
        ExternalServiceArea area(json, true);
        check(area.deriveIntendedServiceAreas().empty(), "civic addresses alone give no Intended Service Area");
    }

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
