/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBS service type attribute rules unit test
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

/* Covers servTypeViolations(), which decides which MBS Distribution Session attributes the parent
 * MBS User Service's servType permits, and which identifier type it may carry.
 *
 * The identifier case is the one with a specification behind it. TS 23.247 V18.8.0 clause 6.5.1
 * gives the MBS Session ID types as "-TMGI (for broadcast and multicast MBS sessions);" and
 * "-source specific IP multicast address (for multicast MBS sessions)", so an SSM identifies a
 * multicast MBS Session only and a BROADCAST MBS User Service cannot be provisioned with one.
 *
 * The attribute cases are TS 29.580's: nrRedCapUeInfo and mbsFSAId broadcast-only, restrictedFlag
 * multicast-only.
 */

#include "ServTypeAttributeRules.hh"

#include <cstdio>
#include <string>

using namespace com::fiveg_mag::ref_tools::mbsf;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; printf("INFO: %s passed.\n", what); }
    else    { g_fail++; printf("FAIL: %s\n", what); }
}

/** @return true if @p attr is among the violations reported for @p serv_type. */
static bool reports(const std::string &serv_type, const ServTypeAttributes &present, const char *attr)
{
    for (const auto &[name, reason] : servTypeViolations(serv_type, present)) {
        if (name == attr) return !reason.empty();
    }
    return false;
}

static size_t count(const std::string &serv_type, const ServTypeAttributes &present)
{
    return servTypeViolations(serv_type, present).size();
}

int main(void)
{
    const ServTypeAttributes none;

    /* Nothing provisioned, nothing to report, whatever the service type is. */
    check(count("BROADCAST", none) == 0, "a session with none of these attributes is accepted as BROADCAST");
    check(count("MULTICAST", none) == 0, "a session with none of these attributes is accepted as MULTICAST");
    check(count("", none) == 0, "a session with none of these attributes is accepted with no service type");

    /* The identifier rule, which is what TS 23.247 clause 6.5.1 decides. */
    ServTypeAttributes ssm;
    ssm.ssmMbsSessionId = true;
    check(reports("BROADCAST", ssm, "mbsSessionId.ssm"),
          "an SSM is refused on a BROADCAST MBS User Service");
    check(!reports("MULTICAST", ssm, "mbsSessionId.ssm"),
          "an SSM is accepted on a MULTICAST MBS User Service");
    check(count("MULTICAST", ssm) == 0,
          "an SSM on a MULTICAST service raises nothing else either");

    /* An unknown or absent service type does not establish that the session is a broadcast one, so
     * the SSM is not refused on a guess. This is the asymmetry the rule documents. */
    check(!reports("", ssm, "mbsSessionId.ssm"),
          "an SSM is not refused when the service type is unknown");
    check(!reports("broadcast", ssm, "mbsSessionId.ssm"),
          "the service type is matched exactly, so lower-case does not trigger the SSM refusal");

    /* The broadcast-only attributes, refused whenever the service type is anything else. */
    ServTypeAttributes redcap;
    redcap.nrRedCapUeInfo = true;
    check(!reports("BROADCAST", redcap, "nrRedCapUeInfo"), "nrRedCapUeInfo is accepted on BROADCAST");
    check(reports("MULTICAST", redcap, "nrRedCapUeInfo"), "nrRedCapUeInfo is refused on MULTICAST");
    check(reports("", redcap, "nrRedCapUeInfo"), "nrRedCapUeInfo is refused when the service type is unknown");

    ServTypeAttributes fsa;
    fsa.mbsFSAId = true;
    check(!reports("BROADCAST", fsa, "mbsFSAId"), "mbsFSAId is accepted on BROADCAST");
    check(reports("MULTICAST", fsa, "mbsFSAId"), "mbsFSAId is refused on MULTICAST");

    /* The multicast-only attribute, the mirror of the two above. */
    ServTypeAttributes restricted;
    restricted.restrictedFlag = true;
    check(!reports("MULTICAST", restricted, "restrictedFlag"), "restrictedFlag is accepted on MULTICAST");
    check(reports("BROADCAST", restricted, "restrictedFlag"), "restrictedFlag is refused on BROADCAST");
    check(reports("", restricted, "restrictedFlag"), "restrictedFlag is refused when the service type is unknown");

    /* Every applicable violation is reported, not just the first, so a caller can put them all in
     * invalidParams in one response. */
    ServTypeAttributes all;
    all.nrRedCapUeInfo = true;
    all.mbsFSAId = true;
    all.restrictedFlag = true;
    all.ssmMbsSessionId = true;
    check(count("BROADCAST", all) == 2, "a BROADCAST session reports both restrictedFlag and the SSM");
    check(reports("BROADCAST", all, "restrictedFlag") && reports("BROADCAST", all, "mbsSessionId.ssm"),
          "and names each of them");
    check(count("MULTICAST", all) == 2, "a MULTICAST session reports both nrRedCapUeInfo and mbsFSAId");
    check(count("", all) == 3, "an unknown service type reports the three attribute rules and not the SSM");

    /* Each violation carries a reason, which is what reaches the client in invalidParams. */
    bool all_have_reasons = true;
    for (const auto &[name, reason] : servTypeViolations("BROADCAST", all)) {
        if (reason.empty() || name.empty()) all_have_reasons = false;
    }
    check(all_have_reasons, "every violation carries a non-empty attribute name and reason");

    printf("Test: ServTypeAttributeRules Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
