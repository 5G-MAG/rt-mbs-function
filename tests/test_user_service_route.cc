/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBS User Services path routing unit test
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

/* Covers UserService::route(), which decides what resource a request path names before any method
 * branch runs.
 *
 * TS 29.580 defines two resources on this API: the MBS User Services collection and an individual
 * MBS User Service. A path with a component after the identifier names neither, and TS 29.500
 * clause 5.2.7.2 answers a target resource that does not exist with 404.
 */

#include "UserService.hh"

#include <cstdio>
#include <cstdlib>

using namespace com::fiveg_mag::ref_tools::mbsf;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; printf("INFO: %s passed.\n", what); }
    else    { g_fail++; printf("FAIL: %s\n", what); }
}

int main(void)
{
    using Route = UserService::Route;

    /* /mbs-user-services */
    check(UserService::route(nullptr, nullptr) == Route::Collection,
          "no identifier names the collection");

    /* /mbs-user-services/{mbsUserServId} */
    check(UserService::route("svc-1", nullptr) == Route::Individual,
          "an identifier alone names an individual service");

    /* /mbs-user-services/{mbsUserServId}/anything -- the case that used to fall through to whichever
       method branch it landed in. This is the reason the rule was extracted. */
    check(UserService::route("svc-1", "ingest-sessions") == Route::NoSuchResource,
          "a component after the identifier names no resource");
    check(UserService::route("svc-1", "subscriptions") == Route::NoSuchResource,
          "any deeper component names no resource, whatever it is called");

    /* An empty-but-present component is still a component: the path had a separator and something
       after it, so it is not the individual resource. */
    check(UserService::route("svc-1", "") == Route::NoSuchResource,
          "an empty component after the identifier still names no resource");

    /* A second component with no first cannot arise from a parsed path, but the rule must not report
       Individual for it if it ever does. */
    check(UserService::route(nullptr, "orphan") == Route::NoSuchResource,
          "a second component with no first names no resource");

    printf("Test: UserService::route Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
