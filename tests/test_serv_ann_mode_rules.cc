/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Service announcement mode rules unit test
 ******************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Authors: Jordi J. Gimenez (5G-MAG)
 * Copyright: (C) 2026 5G-MAG Association
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://www.5g-mag.com/license
 */

/* Covers servAnnPaths(), which says on which of the two paths the MBSF serves an announcement itself
 * (the MBS-5 User Service Description retrieval API and the User Service Announcement Channel) a set of
 * Service announcement modes advertises an MBS User Service.
 *
 * The attribute is 1..* and any combination is valid, so each of the seven combinations of the three
 * modes is checked. TS 26.502 V19.6.0 clause 4.5.3, table 4.5.3-1: "Determines whether the MBS User
 * Service Announcement compiled by the MBSF is advertised to the MBSF Client at reference point MBS-5
 * and/or advertised to the MBSF Client via the MBS Distribution Session and/or passed back to the MBS
 * Application Provider via reference point Nmb10." PASSED_BACK selects neither of the two paths.
 */

#include "ServAnnModeRules.hh"

#include <cstdio>
#include <string>

#include "openapi/model/MBSUserService.h"

using namespace com::fiveg_mag::ref_tools::mbsf;
using reftools::mbsf::MBSUserService;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const std::string &what)
{
    if (ok) { g_pass++; printf("INFO: %s passed.\n", what.c_str()); }
    else    { g_fail++; printf("FAIL: %s\n", what.c_str()); }
}

/** The paths selected by a User Service provisioned with the given servAnnModes (a JSON array). */
static ServAnnPaths pathsOf(const std::string &modes)
{
    const std::string body =
        "{\"extServiceIds\":[\"urn:test:modes\"],\"servType\":\"BROADCAST\","
        "\"servClass\":\"urn:oma:bcast:oma_bsc:st:1.0\",\"servAnnModes\":" + modes + ","
        "\"servNameDescs\":[{\"servName\":\"t\",\"servDescrip\":\"t\",\"language\":\"eng\"}],"
        "\"mainServLang\":\"eng\"}";
    MBSUserService service(body, true);
    return servAnnPaths(service.getServAnnModes());
}

static void expect(const std::string &modes, bool mbs5, bool distribution_session)
{
    const ServAnnPaths paths = pathsOf(modes);
    check(paths.mbs5 == mbs5 && paths.distributionSession == distribution_session,
          modes + " selects MBS-5 " + (mbs5 ? "yes" : "no") + ", Announcement Channel " +
          (distribution_session ? "yes" : "no"));
}

int main(int argc, char *argv[])
{
    /* One mode each. */
    expect("[\"VIA_MBS_5\"]",                                   true,  false);
    expect("[\"VIA_MBS_DISTRIBUTION_SESSION\"]",                false, true);
    expect("[\"PASSED_BACK\"]",                                 false, false);

    /* Two modes: each path follows its own mode, whatever else is provisioned. */
    expect("[\"VIA_MBS_5\",\"VIA_MBS_DISTRIBUTION_SESSION\"]",  true,  true);
    expect("[\"VIA_MBS_5\",\"PASSED_BACK\"]",                   true,  false);
    expect("[\"VIA_MBS_DISTRIBUTION_SESSION\",\"PASSED_BACK\"]", false, true);

    /* All three, in the other order too. */
    expect("[\"VIA_MBS_5\",\"VIA_MBS_DISTRIBUTION_SESSION\",\"PASSED_BACK\"]", true, true);
    expect("[\"PASSED_BACK\",\"VIA_MBS_DISTRIBUTION_SESSION\",\"VIA_MBS_5\"]", true, true);

    /* A mode listed twice selects its path once and does not select the other. */
    expect("[\"VIA_MBS_5\",\"VIA_MBS_5\"]",                      true,  false);

    printf("Test: ServAnnModeRules Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
