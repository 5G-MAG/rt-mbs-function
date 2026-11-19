/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: conditional request evaluation unit test
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

/* Covers evaluatePreconditions() and the entity-tag comparison it rests on.
 *
 * Neither If-Match nor If-None-Match was read by this MBSF: a PUT carrying a stale If-Match was
 * answered 200 and the update applied, so a consumer using the entity-tag for optimistic
 * concurrency silently overwrote a change it had never seen.
 *
 * RFC 9110 section 13.1.1: "An origin server that evaluates an If-Match condition MUST NOT perform
 * the requested method if the condition evaluates to false." Section 13.1.2 requires a matching
 * If-None-Match on a safe method to be answered 304.
 */

#include "ConditionalRequest.hh"

#include <chrono>
#include <cstdio>
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

int main(void)
{
    const std::string tag("abc123");
    const std::string quoted("\"abc123\"");

    /* The opaque-tag is what is compared: RFC 9110 section 8.8.3 wraps it in quotes and may prefix
       a weakness indicator, and neither is part of the value. */
    check(entityTagOpaque("\"abc123\"") == "abc123", "quotes are not part of the entity-tag value");
    check(entityTagOpaque("W/\"abc123\"") == "abc123", "a weak tag compares on the same opaque-tag");
    check(entityTagOpaque("abc123") == "abc123", "an unquoted tag is taken as it stands");

    /* A client sends the tag as the server wrote it, quoted; the stored value here is bare. */
    check(entityTagListMatches(quoted, tag), "a quoted request tag matches the stored bare tag");
    check(entityTagListMatches("\"other\", \"abc123\"", tag), "a list matches on any of its members");
    check(entityTagListMatches("*", tag), "the wildcard matches any existing representation");
    check(!entityTagListMatches("\"stale\"", tag), "a tag that is not the current one does not match");
    check(!entityTagListMatches("", tag), "an empty field value matches nothing");

    /* If-Match, the case that was allowing lost updates. */
    check(evaluatePreconditions("\"stale\"", "", tag, false) == Precondition::PreconditionFailed,
          "a stale If-Match on an unsafe method fails the precondition");
    check(evaluatePreconditions(quoted, "", tag, false) == Precondition::Proceed,
          "a current If-Match lets the method proceed");
    check(evaluatePreconditions("*", "", tag, false) == Precondition::Proceed,
          "If-Match * proceeds when the resource exists");

    /* If-None-Match: 304 for a safe method, a failed precondition otherwise. */
    check(evaluatePreconditions("", quoted, tag, true) == Precondition::NotModified,
          "a matching If-None-Match on a GET is answered 304");
    check(evaluatePreconditions("", "\"stale\"", tag, true) == Precondition::Proceed,
          "a non-matching If-None-Match on a GET returns the representation");
    check(evaluatePreconditions("", quoted, tag, false) == Precondition::PreconditionFailed,
          "a matching If-None-Match on an unsafe method fails the precondition");

    /* No conditions at all, which is every request this MBSF saw before. */
    check(evaluatePreconditions("", "", tag, true) == Precondition::Proceed,
          "a request with neither header proceeds");
    check(evaluatePreconditions("", "", tag, false) == Precondition::Proceed,
          "an unsafe request with neither header proceeds");

    /* If-Match is evaluated first, per RFC 9110 section 13.2.2's evaluation order. */
    check(evaluatePreconditions("\"stale\"", quoted, tag, true) == Precondition::PreconditionFailed,
          "a failing If-Match decides the outcome before If-None-Match is considered");

    /* The Last-Modified value of the MBS-1 bundle: an IMF-fixdate that reads back to the same second. */
    using namespace std::chrono;
    check(httpDate(system_clock::time_point{}) == "Thu, 01 Jan 1970 00:00:00 GMT", "the epoch is written as an IMF-fixdate");
    const auto when = system_clock::time_point{seconds{1700000000}};
    check(httpDate(when) == "Tue, 14 Nov 2023 22:13:20 GMT", "a date after the epoch is written as an IMF-fixdate");
    check(httpDate(when + milliseconds{900}) == "Tue, 14 Nov 2023 22:13:20 GMT", "a fraction of a second is cut, not rounded up");
    check(parseHttpDate(httpDate(when)) == std::optional<std::time_t>(1700000000), "the written date reads back to the same second");

    /* A GET with validators: the tag decides when the client offered one, the date only when it did not. */
    const std::optional<std::string> modified("Wed, 01 Jan 2025 00:00:00 GMT");
    const std::optional<std::string> no_header;
    check(getNotModified(quoted, no_header, tag, modified), "a matching If-None-Match is answered 304");
    check(!getNotModified(std::optional<std::string>("\"stale\""), no_header, tag, modified), "a stale If-None-Match returns the representation");
    check(getNotModified(std::optional<std::string>("*"), no_header, tag, modified), "If-None-Match * is answered 304");
    check(getNotModified(no_header, modified, tag, modified), "If-Modified-Since the modification time is answered 304");
    check(getNotModified(std::optional<std::string>("Thu, 02 Jan 2025 00:00:00 GMT"), no_header, tag, modified) == false,
          "a date is not taken for an entity tag");
    check(getNotModified(no_header, std::optional<std::string>("Thu, 02 Jan 2025 00:00:00 GMT"), tag, modified),
          "If-Modified-Since a later time is answered 304");
    check(!getNotModified(no_header, std::optional<std::string>("Tue, 31 Dec 2024 23:59:59 GMT"), tag, modified),
          "If-Modified-Since an earlier time returns the representation");
    check(!getNotModified(std::optional<std::string>("\"stale\""), modified, tag, modified),
          "a stale If-None-Match decides before a current If-Modified-Since is considered");
    check(!getNotModified(no_header, modified, tag, no_header), "a resource with no modification time is never answered 304 on a date");
    check(!getNotModified(no_header, std::optional<std::string>("not a date"), tag, modified), "an unreadable date is no condition");
    check(!getNotModified(no_header, no_header, tag, modified), "a request without conditions returns the representation");

    printf("Test: ConditionalRequest Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
