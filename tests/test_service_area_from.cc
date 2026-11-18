/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBS Service Area inbound conversion unit test
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

/* Covers ServiceArea::fromServiceArea() and the per-type conversions it uses, the direction that
 * reads an MBS Service Area back out of the mb-smf-service-consumer types, and
 * MBSMFMBSSession::getReducedServiceArea() which exposes the reduced area the MB-SMF reports
 * under TS 29.532 V18.6.0 cl.5.3.2.2.1. The Tac/NrCellId/Nid hex encodings and the MNC digit
 * count are pinned here because nothing else in this component checks them. */

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "mb-smf-service-consumer.h"

#include "MBSMFMBSSession.hh"
#include "ServiceArea.hh"
#include "openapi/model/CJson.hh"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/NcgiTai.h"
#include "openapi/model/Ncgi.h"
#include "openapi/model/Tai.h"
#include "openapi/model/PlmnId.h"

using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::Ncgi;
using reftools::mbsf::Tai;

/* Link seam, not test behaviour: see test_mbsmf_tmgi.cc's own comment on these five
 * UserDataIngSession:: static methods. MBSMFMBSSession.cc reaches them from paths this test
 * never takes, and the real UserDataIngSession.cc needs App::self(). */
MBSF_NAMESPACE_START
bool UserDataIngSession::tmgi(mb_smf_sc_tmgi_t *, const UserDataIngDistSessId &) { return true; }
void UserDataIngSession::setMBSSessionFlag(const UserDataIngDistSessId &) {}
void UserDataIngSession::setMBSSessionDeleted(const UserDataIngDistSessId &) {}
void UserDataIngSession::setMBSSessionFailureFlag(const UserDataIngDistSessId &,
        const std::optional<fiveg_mag_reftools::ProblemCause> &,
        const std::optional<fiveg_mag_reftools::CJson> &) {}
void UserDataIngSession::setMBSSessionUpdateResult(const UserDataIngDistSessId &,
        const std::optional<fiveg_mag_reftools::ProblemCause> &,
        const std::optional<fiveg_mag_reftools::CJson> &) {}
MBSF_NAMESPACE_STOP

static size_t total = 0, failed = 0;

#define CHECK(cond, msg) do { \
    total++; \
    if (!(cond)) { failed++; std::fprintf(stderr, "FAILED: %s\n", msg); } \
    else { std::fprintf(stderr, "OK: %s\n", msg); } \
} while (0)

/* MNC "001" is deliberate: three digits but numerically below 100, the case a conversion that
 * derives the digit count from the numeric value gets wrong. */
static mb_smf_sc_mbs_service_area_t *make_area()
{
    mb_smf_sc_mbs_service_area_t *area = mb_smf_sc_mbs_service_area_new();

    mb_smf_sc_tai_t *tai = mb_smf_sc_tai_new_len(234, 1, 3, 0xABCD, nullptr);
    ogs_list_add(&area->tais, tai);

    uint64_t nid = 0x2A;
    mb_smf_sc_ncgi_t *ncgi = mb_smf_sc_ncgi_new();
    mb_smf_sc_ncgi_set_plmn_id_len(ncgi, 310, 26, 2);
    ncgi->nr_cell_id = 0x123456789ULL;
    ncgi->nid = static_cast<uint64_t*>(ogs_malloc(sizeof(*ncgi->nid)));
    *ncgi->nid = nid;

    mb_smf_sc_tai_t *cell_tai = mb_smf_sc_tai_new_len(310, 26, 2, 0x112233, nullptr);
    mb_smf_sc_ncgi_tai_t *ncgi_tai = mb_smf_sc_ncgi_tai_new_values(cell_tai, ncgi);
    ogs_list_add(&area->ncgi_tais, ncgi_tai);

    mb_smf_sc_tai_free(cell_tai);
    mb_smf_sc_ncgi_delete(ncgi);

    return area;
}

int main(int argc, char *argv[])
{
    /* NR NTN Intended Service Areas (TS 29.571 V19.8.0 table 5.9.4.4-1, intendedServAreaList): the octets travel between the MbsServiceArea
     * model and the library type unchanged, in both directions, and an area with none stays without the attribute. */
    {
        static const unsigned char first[] = {0x01, 0x02, 0x03, 0xfe, 0xff};
        static const unsigned char second[] = {0xa0};
        std::shared_ptr<MbsServiceArea> model(new MbsServiceArea());
        // the model holds the base64 text of the octets (01 02 03 fe ff, and a0), as the JSON parser leaves it
        const std::basic_string<unsigned char> first_text(reinterpret_cast<const unsigned char*>("AQID/v8="), 8);
        const std::basic_string<unsigned char> second_text(reinterpret_cast<const unsigned char*>("oA=="), 4);
        model->addIntendedServAreaList(first_text);
        model->addIntendedServAreaList(second_text);
        MBSF_NAMESPACE::ServiceArea service_area(model);
        mb_smf_sc_mbs_service_area_t *lib = service_area.populateServiceArea();

        CHECK(lib && ogs_list_count(&lib->intended_service_areas) == 2, "populateServiceArea() carries both Intended Service Areas");
        if (lib && ogs_list_count(&lib->intended_service_areas) == 2) {
            mb_smf_sc_intended_service_area_t *a = (mb_smf_sc_intended_service_area_t*)ogs_list_first(&lib->intended_service_areas);
            mb_smf_sc_intended_service_area_t *b = (mb_smf_sc_intended_service_area_t*)ogs_list_next(a);
            CHECK(a->coordinates_len == sizeof(first) && memcmp(a->coordinates, first, sizeof(first)) == 0,
                  "the first Intended Service Area keeps its octets");
            CHECK(b->coordinates_len == sizeof(second) && memcmp(b->coordinates, second, sizeof(second)) == 0,
                  "the second Intended Service Area keeps its octets");

            std::shared_ptr<MbsServiceArea> back = MBSF_NAMESPACE::ServiceArea::fromServiceArea(lib);
            const auto &list = back ? back->getIntendedServAreaList() : MbsServiceArea::IntendedServAreaListType();
            CHECK(list.has_value() && list.value().size() == 2, "fromServiceArea() returns both Intended Service Areas");
            if (list.has_value() && list.value().size() == 2) {
                CHECK(list.value().front().value() == first_text, "the first Intended Service Area is returned as the same base64 text");
            }
        }
        if (lib) mb_smf_sc_mbs_service_area_delete(lib);

        std::shared_ptr<MbsServiceArea> none(new MbsServiceArea());
        MBSF_NAMESPACE::ServiceArea no_isa(none);
        mb_smf_sc_mbs_service_area_t *lib_none = no_isa.populateServiceArea();
        CHECK(lib_none && ogs_list_count(&lib_none->intended_service_areas) == 0, "an area without Intended Service Areas gives none");
        std::shared_ptr<MbsServiceArea> back_none = MBSF_NAMESPACE::ServiceArea::fromServiceArea(lib_none);
        CHECK(back_none && !back_none->getIntendedServAreaList().has_value(), "and none are written back");
        if (lib_none) mb_smf_sc_mbs_service_area_delete(lib_none);
    }

    /* Case 1: nothing in, nothing out. An MBS Session the MB-SMF did not reduce must not
     * produce an empty MbsServiceArea that a reader could mistake for "reduced to nothing". */
    {
        CHECK(!MBSF_NAMESPACE::ServiceArea::fromServiceArea(nullptr),
              "fromServiceArea(nullptr) yields no MbsServiceArea");
    }

    /* Case 2: the values and their encodings. */
    {
        mb_smf_sc_mbs_service_area_t *area = make_area();
        std::shared_ptr<MbsServiceArea> model = MBSF_NAMESPACE::ServiceArea::fromServiceArea(area);

        CHECK(!!model, "fromServiceArea() converts a populated service area");
        if (model) {
            const auto &tai_list = model->getTaiList();
            CHECK(tai_list.has_value() && tai_list.value().size() == 1,
                  "the single plain TAI is carried across");
            if (tai_list.has_value() && tai_list.value().size() == 1) {
                std::shared_ptr<Tai> tai = tai_list.value().front().value();
                CHECK(tai->getPlmnId()->getMcc() == "234", "the TAI's MCC is carried across");
                CHECK(tai->getPlmnId()->getMnc() == "001",
                      "a 3-digit MNC below 100 keeps all three digits, not two");
                CHECK(tai->getTac() == "ABCD",
                      "Tac is upper-case hex padded to 4 digits, the encoding the library writes");
                CHECK(!tai->getNid().has_value(), "an absent Nid stays absent");
            }

            const auto &ncgi_list = model->getNcgiList();
            CHECK(ncgi_list.has_value() && ncgi_list.value().size() == 1,
                  "the single NCGI TAI is carried across");
            if (ncgi_list.has_value() && ncgi_list.value().size() == 1) {
                std::shared_ptr<NcgiTai> ncgi_tai = ncgi_list.value().front().value();
                CHECK(ncgi_tai->getTai()->getTac() == "112233",
                      "a 6-digit Tac is not truncated to 4");
                CHECK(ncgi_tai->getCellList().size() == 1, "the NCGI TAI's single cell is carried across");
                if (ncgi_tai->getCellList().size() == 1) {
                    std::shared_ptr<Ncgi> cell = ncgi_tai->getCellList().front().value();
                    CHECK(cell->getNrCellId() == "123456789",
                          "NrCellId is upper-case hex padded to its full 9 digits");
                    CHECK(cell->getNid().has_value() && cell->getNid().value() == "0000000002A",
                          "Nid is upper-case hex padded to its full 11 digits");
                }
            }
        }
        mb_smf_sc_mbs_service_area_delete(area);
    }

    /* Case 2b: values narrower than their field. Tac and NrCellId are fixed-width hex in the
     * encoding the library writes, so a small value is zero-padded rather than shortened. The
     * values in case 2 are all exactly as wide as their field and cannot show this. */
    {
        mb_smf_sc_mbs_service_area_t *area = mb_smf_sc_mbs_service_area_new();
        ogs_list_add(&area->tais, mb_smf_sc_tai_new_len(234, 1, 3, 0x42, nullptr));

        mb_smf_sc_ncgi_t *ncgi = mb_smf_sc_ncgi_new();
        mb_smf_sc_ncgi_set_plmn_id_len(ncgi, 234, 1, 3);
        ncgi->nr_cell_id = 0x1ULL;
        mb_smf_sc_tai_t *cell_tai = mb_smf_sc_tai_new_len(234, 1, 3, 0x42, nullptr);
        ogs_list_add(&area->ncgi_tais, mb_smf_sc_ncgi_tai_new_values(cell_tai, ncgi));
        mb_smf_sc_tai_free(cell_tai);
        mb_smf_sc_ncgi_delete(ncgi);

        std::shared_ptr<MbsServiceArea> model = MBSF_NAMESPACE::ServiceArea::fromServiceArea(area);
        CHECK(!!model, "a service area of narrow values converts");
        if (model) {
            std::shared_ptr<Tai> tai = model->getTaiList().value().front().value();
            CHECK(tai->getTac() == "0042", "a Tac below 0x1000 is padded to 4 digits, not shortened");

            std::shared_ptr<NcgiTai> ncgi_tai = model->getNcgiList().value().front().value();
            std::shared_ptr<Ncgi> cell = ncgi_tai->getCellList().front().value();
            CHECK(cell->getNrCellId() == "000000001",
                  "a small NrCellId is padded to 9 digits, not shortened");
        }
        mb_smf_sc_mbs_service_area_delete(area);
    }

    /* Case 3: out and back again. populateServiceArea() is the direction this component already
     * had; a value that survives both is encoded the same way in each. */
    {
        mb_smf_sc_mbs_service_area_t *area = make_area();
        std::shared_ptr<MbsServiceArea> model = MBSF_NAMESPACE::ServiceArea::fromServiceArea(area);
        mb_smf_sc_mbs_service_area_delete(area);

        CHECK(!!model, "round trip: the first conversion produced a model");
        if (model) {
            MBSF_NAMESPACE::ServiceArea service_area(model);
            mb_smf_sc_mbs_service_area_t *back = service_area.populateServiceArea();
            CHECK(!!back, "round trip: the model converts back to the library type");
            if (back) {
                std::shared_ptr<MbsServiceArea> again = MBSF_NAMESPACE::ServiceArea::fromServiceArea(back);
                /* Compared as serialised JSON, not with MbsServiceArea::operator==. That operator
                 * compares its NcgiList and TaiList as std::list<std::optional<std::shared_ptr<T>>>,
                 * and shared_ptr's own operator== compares addresses, so two independently built
                 * models of the same service area never compare equal through it. */
                CHECK(!!again && again->toJSON(false).serialise() == model->toJSON(false).serialise(),
                      "round trip: a service area is unchanged by a conversion out and back");
                mb_smf_sc_mbs_service_area_delete(back);
            }
        }
    }

    /* Case 4: the reduced area reaches the session wrapper, and its absence is distinguishable.
     * Session deliberately leaked, see test_mbsmf_tmgi.cc's own comment on why. */
    {
        mb_smf_sc_mbs_session_t *raw = mb_smf_sc_mbs_session_new();
        auto *session = new MBSF_NAMESPACE::MBSMFMBSSession(raw);

        CHECK(!session->getReducedServiceArea(),
              "a session the MB-SMF did not reduce reports no reduced service area");

        raw->red_mbs_service_area = make_area();

        std::shared_ptr<MbsServiceArea> reduced = session->getReducedServiceArea();
        CHECK(!!reduced, "a reduced service area on the session is reported by the wrapper");
        if (reduced) {
            CHECK(reduced->getTaiList().has_value() && reduced->getTaiList().value().size() == 1,
                  "the reported reduced service area carries the TAI the MB-SMF kept");
        }

        /* Released even though the session it hangs off is not, so that a leak check over this
         * test reports only the session and its wrapper. Anything else it reports is then a real
         * leak in the conversions this file covers. */
        mb_smf_sc_mbs_service_area_delete(raw->red_mbs_service_area);
        raw->red_mbs_service_area = nullptr;
    }

    std::fprintf(stderr, "%zu/%zu checks passed\n", total - failed, total);
    return failed == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
