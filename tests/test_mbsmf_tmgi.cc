/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBSMFMBSSession TMGI passthrough unit test
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

/* Covers MBSMFMBSSession::setTmgi() (AF-supplied TMGI passthrough to the vendored
 * mb-smf-service-consumer library) and the pre-existing setTmgiRequest() path it must not
 * regress. See the commit introducing this file for the full citation trail (TS 23.247
 * V18.8.0 cl.7.1.1.2 step 8; TS 29.580 V18.8.0 cl.5.3.2.2.2; mb-smf-service-consumer's own
 * mbs-session.h contract for mb_smf_sc_mbs_session_set_tmgi()). */

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"
#include "openapi/model/nf_type.h"

#include <cstdio>
#include <string>

#include "mb-smf-service-consumer.h"

#include "MBSMFMBSSession.hh"
#include "openapi/model/Tmgi.h"
#include "openapi/model/PlmnId.h"

using reftools::mbsf::Tmgi;
using reftools::mbsf::PlmnId;

/* Link seam, not test behaviour: MBSMFMBSSession.cc's own processEvent()/
 * sendLocalNotifyEvent()/mbsSessionNotifyCallback() (none of which this test calls) reach four
 * UserDataIngSession:: static methods, and the real UserDataIngSession.cc that defines them
 * needs App::self() -- a singleton this test deliberately never constructs (App::App() parses a
 * full mbsf.yaml and opens real SBI listener sockets, App.cc's own initialise(); unsuitable for
 * a unit test, and unsafe to run here while a real MBSF may be listening on those same ports).
 * Providing narrow stand-ins for exactly these four declared-but-otherwise-unreachable-from-this-
 * test methods keeps the real UserDataIngSession.cc/App.cc out of this binary's link entirely.
 * Bodies are never exercised by any path this test takes. */
MBSF_NAMESPACE_START
bool UserDataIngSession::tmgi(mb_smf_sc_tmgi_t *, const UserDataIngDistSessId &) { return true; }
void UserDataIngSession::setMBSSessionFlag(const UserDataIngDistSessId &) {}
void UserDataIngSession::setMBSSessionDeleted(const UserDataIngDistSessId &) {}
void UserDataIngSession::setMBSSessionFailureFlag(const UserDataIngDistSessId &,
        const std::optional<fiveg_mag_reftools::ProblemCause> &,
        const std::optional<fiveg_mag_reftools::CJson> &) {}
MBSF_NAMESPACE_STOP

/* Intentionally never destroyed: MBSMFMBSSession's own destructor calls deleteSession(),
 * which reaches mb_smf_sc_mbs_session_push_changes() -> _mbs_session_send_create()/
 * _mbs_session_send_remove() (mbs-session.c) -- real SBI NF discovery and request sending,
 * needing a running SBI client stack this standalone unit test does not set up. Leaking the
 * wrapper (and the raw session it owns) is deliberate: the process exits immediately after
 * the assertions below, and constructing that stack would test the vendored library's own
 * transport code, not the call-site logic this file is for. */
static MBSF_NAMESPACE::MBSMFMBSSession *make_session()
{
    mb_smf_sc_mbs_session_t *raw = mb_smf_sc_mbs_session_new();
    return new MBSF_NAMESPACE::MBSMFMBSSession(raw);
}

static std::shared_ptr<Tmgi> make_af_supplied_tmgi()
{
    auto plmn = std::make_shared<PlmnId>();
    plmn->setMcc("001");
    plmn->setMnc("01");

    auto tmgi = std::make_shared<Tmgi>();
    tmgi->setMbsServiceId("9763CA");
    tmgi->setPlmnId(plmn);
    return tmgi;
}

int main(int argc, char *argv[])
{
    ogs_app_initialize("1.0.0", "test_mbsmf_tmgi.yaml", (const char* const*)argv);

    /* mb-smf-service-consumer's own module context (context.c's static __self) is only
     * created by mb_smf_sc_parse_config() (Open5GSNetworkFunction::initialise() calls this for
     * the real MBSF, ogs-sbi.h's ogs_sbi_context_init() first, matching the real init order) --
     * without it, _context_add_tmgi() (context.c) dereferences a null context with no guard
     * (unlike _context_add_mbs_session()'s own null check), so mb_smf_sc_tmgi_new() alone
     * segfaults. Confirmed by direct read of context.c: this is a real, separate defect in
     * rt-5gc-service-consumers -- not exercised in the real MBSF, which always calls
     * mb_smf_sc_parse_config() during startup, but real for any caller (such as a unit test)
     * that constructs a TMGI without it. Recorded in rt-5gc-service-consumers's own register,
     * not fixed here (out of this repository's scope, see the introducing commit). */
    ogs_sbi_context_init(OpenAPI_nf_type_AF);
    mb_smf_sc_parse_config("mbsmfsc");

    size_t total = 0, failed = 0;

#define CHECK(cond, msg) do { \
    total++; \
    if (!(cond)) { failed++; std::fprintf(stderr, "FAILED: %s\n", msg); } \
    else { std::fprintf(stderr, "OK: %s\n", msg); } \
} while (0)

    /* Case 1: an AF-supplied TMGI must reach the underlying mb_smf_sc_mbs_session_t via
     * setTmgi(), not merely setTmgiRequest(false). This is the defect this file's own
     * introducing commit fixes: previously nothing on this call path could reach here at
     * all (no ContextData field, no setTmgi() wrapper). */
    {
        mb_smf_sc_mbs_session_t *raw = mb_smf_sc_mbs_session_new();
        auto *session = new MBSF_NAMESPACE::MBSMFMBSSession(raw);

        CHECK(raw->tmgi_req == true, "fresh session defaults to tmgi_req=true (library default, precondition)");

        session->setTmgi(make_af_supplied_tmgi());

        CHECK(raw->tmgi != nullptr, "setTmgi() attaches a non-null mb_smf_sc_tmgi_t to the session");
        CHECK(raw->tmgi_req == false,
              "setTmgi() clears tmgi_req (mb-smf-service-consumer's own mutual-exclusivity contract)");
        if (raw->tmgi) {
            CHECK(raw->tmgi->mbs_service_id != nullptr
                  && std::string(raw->tmgi->mbs_service_id) == "9763CA",
                  "the attached TMGI carries the mbsServiceId that was parsed from the request");
            CHECK(ogs_plmn_id_mcc(&raw->tmgi->plmn) == 1 && ogs_plmn_id_mnc(&raw->tmgi->plmn) == 1,
                  "the attached TMGI carries the parsed PLMN's MCC/MNC");
        }
        /* session/raw deliberately leaked, see make_session()'s own comment. */
    }

    /* Case 2: no regression -- a request with no AF-supplied TMGI and TMGI allocation
     * requested must still call setTmgiRequest(true) exactly as before this file's own
     * introducing commit (createMbsSession()'s request_tmgi branch, added earlier by F7). */
    {
        auto *session = make_session();

        session->setTmgiRequest(true);

        CHECK(session->getTmgiRequest() == true,
              "setTmgiRequest(true) still requests TMGI allocation when no AF-supplied TMGI is present");
    }

    std::fprintf(stderr, "%zu/%zu checks passed\n", total - failed, total);
    return failed == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
