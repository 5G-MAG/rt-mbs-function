/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Availability Information target service areas unit test
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

/* The service areas of an MBS Distribution Session in the announcement. TS 26.517 V19.2.0 table 5.2.9-1: "This property is
 * deprecated. The property shall be absent in this release." for mbsServiceAreas, and targetServiceAreas, with table 5.2.9-1a:
 * "Exactly one of the following properties shall be present: ncgiList, taiList, geographicAreaList." */

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <cstdio>
#include <memory>
#include <string>

#include "AvailabilityInfo.hh"
#include "ServiceArea.hh"
#include "openapi/model/CJson.hh"

using fiveg_mag_reftools::CJson;

static size_t total = 0, failed = 0;

#define CHECK(cond, msg) do { \
    total++; \
    if (!(cond)) { failed++; std::fprintf(stderr, "FAILED: %s\n", msg); } \
    else { std::fprintf(stderr, "OK: %s\n", msg); } \
} while (0)

static const char *plmn = R"("plmnId":{"mcc":"262","mnc":"01"})";

static std::string cell(const char *nr_cell_id)
{
    return std::string("{") + plmn + R"(,"nrCellId":")" + nr_cell_id + R"("})";
}

static std::string ncgi_tai(const char *tac, const std::string &cells)
{
    return std::string(R"({"tai":{)") + plmn + R"(,"tac":")" + tac + R"("},"cellList":[)" + cells + "]}";
}

static std::string tai(const char *tac)
{
    return std::string("{") + plmn + R"(,"tac":")" + tac + R"("})";
}

static CJson announcement(const std::string &mbs_service_area_json, bool &built)
{
    CJson area_json(CJson::parse(mbs_service_area_json));
    std::shared_ptr<MBSF_NAMESPACE::ServiceArea> area(new MBSF_NAMESPACE::ServiceArea(area_json, false));
    MBSF_NAMESPACE::AvailabilityInfo info(area);
    built = true;
    return info.json(false);
}

static bool has_key(const CJson &object, const char *name)
{
    for (std::size_t i = 0; i < object.arraySize(); i++) {
        const char *key = object.index(i).key();
        if (key && std::string(key) == name) return true;
    }
    return false;
}

int main()
{
    /* Cells of two tracking areas and a list of tracking areas: two target service areas, one of each kind. */
    {
        bool built = false;
        CJson json = announcement(std::string(R"({"ncgiList":[)") + ncgi_tai("000001", cell("000000001") + "," + cell("000000002")) + "," +
                                  ncgi_tai("000002", cell("000000003")) + R"(],"taiList":[)" + tai("000003") + "," + tai("000004") + "]}", built);
        CHECK(built && json.isObject(), "the announcement of an area with cells and tracking areas is built");
        CHECK(!has_key(json, "mbsServiceAreas"), "mbsServiceAreas is absent");
        CHECK(has_key(json, "targetServiceAreas"), "targetServiceAreas is present");

        CJson targets = json.index(0);
        for (std::size_t i = 0; i < json.arraySize(); i++) {
            const char *key = json.index(i).key();
            if (key && std::string(key) == "targetServiceAreas") targets = json.index(i);
        }
        CHECK(targets.isArray() && targets.arraySize() == 2, "an area with cells and tracking areas gives two target service areas");
        if (targets.isArray() && targets.arraySize() == 2) {
            CJson first = targets.index(0), second = targets.index(1);
            CHECK(first.isObject() && first.arraySize() == 1 && std::string(first.index(0).key()) == "ncgiList",
                  "the first target service area has exactly one property, ncgiList");
            CHECK(second.isObject() && second.arraySize() == 1 && std::string(second.index(0).key()) == "taiList",
                  "the second target service area has exactly one property, taiList");
            CHECK(first.index(0).arraySize() == 3, "the ncgiList holds the three cells of both tracking areas");
            CHECK(second.index(0).arraySize() == 2, "the taiList holds both tracking areas");
        }
    }

    /* Tracking areas only: one target service area. */
    {
        bool built = false;
        CJson json = announcement(std::string(R"({"taiList":[)") + tai("000005") + "]}", built);
        CJson targets = CJson::newArray();
        for (std::size_t i = 0; i < json.arraySize(); i++) {
            const char *key = json.index(i).key();
            if (key && std::string(key) == "targetServiceAreas") targets = json.index(i);
        }
        CHECK(!has_key(json, "mbsServiceAreas"), "mbsServiceAreas is absent for tracking areas only");
        CHECK(targets.isArray() && targets.arraySize() == 1 && std::string(targets.index(0).index(0).key()) == "taiList",
              "tracking areas only give one target service area, a taiList");
    }

    /* Nothing to announce: no service area property at all, not an empty one. */
    {
        bool built = false;
        CJson json = announcement("{}", built);
        CHECK(!has_key(json, "mbsServiceAreas") && !has_key(json, "targetServiceAreas"),
              "an area with no cells and no tracking areas announces no service area property");
    }

    std::fprintf(stderr, "%zu checks, %zu failed\n", total, failed);
    return failed == 0 ? 0 : 1;
}
