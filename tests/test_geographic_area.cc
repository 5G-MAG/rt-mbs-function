/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: GeographicArea model unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

/* Covers the GeographicArea model, which TS 29.572 defines as an anyOf of seven shapes, each requiring its
 * own attributes. The generated class holds the members of all seven. It used to require every shape's
 * attributes of every shape, so a POINT was refused for lacking "uncertainty", and it wrote every member
 * back, so a value that did parse came out carrying the other shapes' members. The model template now
 * accepts a value that the alternative named by "shape" accepts, and writes only that alternative.
 */

#include "openapi/model/GeographicArea.h"
#include "openapi/model/ModelException.hh"

#include <cstdio>
#include <set>
#include <string>

using namespace fiveg_mag_reftools;
using reftools::mbsf::GeographicArea;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

static const char *POINT = "{\"lon\":12.5,\"lat\":41.9}";

static bool accepts(const std::string &text)
{
    try {
        CJson json = CJson::parse(text);
        GeographicArea area(json, true);
        return true;
    } catch (const ModelException &) {
        return false;
    }
}

/* The whole-class validator must look only at the alternative the shape names: the members of the others
 * hold defaults that their own limits refuse (an uncertainty of zero, for one). */
static bool validates(const std::string &text)
{
    try {
        CJson json = CJson::parse(text);
        GeographicArea area(json, true);
        return area.validate();
    } catch (const ModelException &) {
        return false;
    }
}

static std::set<std::string> keys_after_round_trip(const std::string &text)
{
    std::set<std::string> keys;
    CJson json = CJson::parse(text);
    GeographicArea area(json, true);
    const CJson out = area.toJSON(true);
    for (std::size_t i = 0; i < out.arraySize(); i++) keys.insert(out.index(i).key());
    return keys;
}

int main()
{
    const std::string point = POINT;

    check(accepts("{\"shape\":\"POINT\",\"point\":" + point + "}"), "a POINT with its point is accepted");
    check(!accepts("{\"shape\":\"POINT\"}"), "a POINT without its point is refused");
    check(accepts("{\"shape\":\"POINT_UNCERTAINTY_CIRCLE\",\"point\":" + point + ",\"uncertainty\":10.0}"),
          "a circle with point and uncertainty is accepted");
    check(!accepts("{\"shape\":\"POINT_UNCERTAINTY_CIRCLE\",\"point\":" + point + "}"),
          "a circle without its uncertainty is refused although a POINT alternative would take it");
    check(accepts("{\"shape\":\"POLYGON\",\"pointList\":[" + point + "," + point + "," + point + "]}"),
          "a polygon with its point list is accepted");
    check(!accepts("{\"shape\":\"POLYGON\",\"point\":" + point + "}"),
          "a polygon without its point list is refused although a POINT alternative would take it");
    check(!accepts("{\"shape\":\"NOT_A_SHAPE\"}"), "an unknown shape that no alternative accepts is refused");
    check(!accepts("{\"point\":" + point + "}"), "a value with no shape is refused");

    {
        const std::set<std::string> keys = keys_after_round_trip("{\"shape\":\"POINT\",\"point\":" + point + "}");
        check(keys == std::set<std::string>({"shape", "point"}), "a POINT is written back as shape and point only");
    }
    {
        const std::set<std::string> keys = keys_after_round_trip(
            "{\"shape\":\"POINT_UNCERTAINTY_CIRCLE\",\"point\":" + point + ",\"uncertainty\":10.0}");
        check(keys == std::set<std::string>({"shape", "point", "uncertainty"}),
              "a circle is written back as shape, point and uncertainty only");
    }
    {
        const std::set<std::string> keys = keys_after_round_trip(
            "{\"shape\":\"POLYGON\",\"pointList\":[" + point + "," + point + "," + point + "]}");
        check(keys == std::set<std::string>({"shape", "pointList"}), "a polygon is written back as shape and pointList only");
    }

    check(validates("{\"shape\":\"POINT\",\"point\":" + point + "}"), "a POINT validates");
    check(validates("{\"shape\":\"POINT_UNCERTAINTY_CIRCLE\",\"point\":" + point + ",\"uncertainty\":10.0}"),
          "a circle validates");
    check(validates("{\"shape\":\"POLYGON\",\"pointList\":[" + point + "," + point + "," + point + "]}"),
          "a polygon validates");

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
