/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Accept header media-type matching unit test
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

/* Covers NfServer::acceptsMediaType(), which decides whether the single representation this NF has
 * to offer is acceptable to the requester.
 *
 * RFC 9110 section 12.5.1 is why an absent header accepts anything: "A request without any Accept
 * header field implies that the user agent will accept any media type in response". Only a
 * present-and-incompatible Accept makes a response unacceptable.
 */

#include "NfServer.hh"

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

using namespace com::fiveg_mag::ref_tools::mbsf;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; printf("INFO: %s passed.\n", what); }
    else    { g_fail++; printf("FAIL: %s\n", what); }
}

static bool accepts(const std::optional<std::string> &hdr, const char *type)
{
    return NfServer::acceptsMediaType(hdr, std::string(type));
}

int main(void)
{
    const char *json = "application/json";

    /* Absent or empty: anything is acceptable. */
    check(accepts(std::nullopt, json), "an absent Accept accepts anything");
    check(accepts(std::string(""), json), "an empty Accept accepts anything");

    /* Exact and wildcard matches. */
    check(accepts(std::string("application/json"), json), "an exact match is accepted");
    check(accepts(std::string("*/*"), json), "*/* is accepted");
    check(accepts(std::string("application/*"), json), "a subtype wildcard is accepted");

    /* Case is not significant in a media type, which is the point of the traits. */
    check(accepts(std::string("APPLICATION/JSON"), json), "an upper-case range matches");
    check(accepts(std::string("Application/Json"), json), "a mixed-case range matches");
    check(accepts(std::string("APPLICATION/*"), json), "an upper-case subtype wildcard matches");

    /* Parameters are ignored: this NF has one representation, so a q value cannot change the answer. */
    check(accepts(std::string("application/json;q=0.5"), json), "a q parameter does not prevent a match");
    check(accepts(std::string("application/json; charset=utf-8"), json), "a charset parameter is ignored");

    /* Lists, whitespace, and empty elements. */
    check(accepts(std::string("text/html, application/json"), json), "a later element in a list matches");
    check(accepts(std::string("  text/html ,  application/json  "), json), "surrounding whitespace is trimmed");
    check(accepts(std::string("text/html,,application/json"), json), "an empty list element is skipped");
    check(accepts(std::string("text/html, */*"), json), "*/* later in a list matches");

    /* Genuine mismatches. */
    check(!accepts(std::string("text/html"), json), "a different type is not accepted");
    check(!accepts(std::string("text/*"), json), "a wildcard on a different type is not accepted");
    check(!accepts(std::string("application/xml"), json), "a different subtype is not accepted");
    check(!accepts(std::string("text/html, application/xml"), json), "a list of mismatches is not accepted");

    /* A subtype wildcard must be exactly '*', not a prefix of a real subtype. */
    check(!accepts(std::string("application/*+xml"), json), "a wildcard-looking subtype is not a wildcard");

    printf("Test: acceptsMediaType Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
