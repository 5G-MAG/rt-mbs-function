/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: announcement modes unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

/* Covers which announcement modes need the bundle of an Ingest Session to be written and which services are for the MBS Application Provider
 * only (TS 26.517 V19.2.0 clause 8A.2), for all eight combinations of the three modes.
 */

#include <cstdio>
#include <string>

#include "AnnouncementModes.hh"

using namespace com::fiveg_mag::ref_tools::mbsf;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const std::string &what)
{
    if (ok) {
        g_pass++;
        std::printf("INFO: %s passed.\n", what.c_str());
    } else {
        g_fail++;
        std::printf("ERROR: %s failed.\n", what.c_str());
    }
}

static announcementmodes::Set modes(bool mbs5, bool dist, bool passed_back)
{
    announcementmodes::Set set;
    set.viaMbs5 = mbs5;
    set.viaDistributionSession = dist;
    set.passedBack = passed_back;
    return set;
}

int main(int argc, char *argv[])
{
    /* A bundle is written for each mode alone, and for every combination. */
    check(announcementmodes::needsBundle(modes(true, false, false)), "VIA_MBS_5 needs the bundle");
    check(announcementmodes::needsBundle(modes(false, true, false)), "VIA_MBS_DISTRIBUTION_SESSION needs the bundle");
    check(announcementmodes::needsBundle(modes(false, false, true)), "PASSED_BACK needs the bundle: it is made available at MBS-1");
    check(announcementmodes::needsBundle(modes(true, true, true)) && announcementmodes::needsBundle(modes(true, false, true)) &&
              announcementmodes::needsBundle(modes(false, true, true)) && announcementmodes::needsBundle(modes(true, true, false)),
          "every combination of two or three modes needs the bundle");
    check(!announcementmodes::needsBundle(modes(false, false, false)), "no mode, no bundle");

    /* Only a service with PASSED_BACK and nothing else is for the application provider alone. */
    check(announcementmodes::passedBackOnly(modes(false, false, true)), "PASSED_BACK alone is for the application provider only");
    check(!announcementmodes::passedBackOnly(modes(true, false, true)), "PASSED_BACK with VIA_MBS_5 is also advertised at MBS-5");
    check(!announcementmodes::passedBackOnly(modes(false, true, true)), "PASSED_BACK with VIA_MBS_DISTRIBUTION_SESSION is also on the channel");
    check(!announcementmodes::passedBackOnly(modes(true, true, true)), "all three modes is not for the application provider only");
    check(!announcementmodes::passedBackOnly(modes(true, false, false)) && !announcementmodes::passedBackOnly(modes(false, true, false)) &&
              !announcementmodes::passedBackOnly(modes(false, false, false)),
          "without PASSED_BACK a service is never for the application provider only");

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
