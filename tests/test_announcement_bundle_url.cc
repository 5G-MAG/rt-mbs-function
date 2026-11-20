/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: announcement bundle URLs unit test
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

/* Covers the two URLs of an Ingest Session's announcement bundle: the object locator the MBSTF pulls, which is unchanged,
 * and the MBS-1 URL derived from it that is passed back to the MBS Application Provider (TS 26.517 V19.2.0 clause 8A.2).
 */

#include <cstdio>
#include <optional>
#include <string>

#include "AnnouncementBundleUrl.hh"

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

int main(int argc, char *argv[])
{
    const std::string locator = bundleurl::objectLocator("127.0.0.67:8888", "abc");

    /* The object locator is the string the MBSF has always given the MBSTF. */
    check(locator == "http://127.0.0.67:8888/x-5gmag-service-announcements/v1/user-data-ingest-session/abc",
          "the object locator is unchanged");

    /* The MBS-1 URL names the same session and the same server, on the MBS-1 path, as a directory URL. */
    check(bundleurl::mbs1UrlFromObjectLocator(locator) ==
              std::optional<std::string>("http://127.0.0.67:8888/x-5gmag-service-announcements/v1/mbs-1/user-data-ingest-session/abc/"),
          "the MBS-1 URL is the same session on the MBS-1 path");
    check(bundleurl::mbs1UrlFromObjectLocator("http://[fd00::1]:8888/x-5gmag-service-announcements/v1/user-data-ingest-session/9e3a") ==
              std::optional<std::string>("http://[fd00::1]:8888/x-5gmag-service-announcements/v1/mbs-1/user-data-ingest-session/9e3a/"),
          "an IPv6 authority is kept");

    /* The two paths must not overlap, or the path delegator could not tell them apart. */
    check(!bundleurl::mbs1Path().starts_with(bundleurl::objectLocatorPath()) && !bundleurl::objectLocatorPath().starts_with(bundleurl::mbs1Path()),
          "neither path is a prefix of the other");

    /* A string that is not one of our locators gives no URL. */
    check(!bundleurl::mbs1UrlFromObjectLocator("http://example.org/other/path/abc"), "a foreign URL gives no MBS-1 URL");
    check(!bundleurl::mbs1UrlFromObjectLocator("ftp://h/x-5gmag-service-announcements/v1/user-data-ingest-session/abc"), "a non-http URL gives none");
    check(!bundleurl::mbs1UrlFromObjectLocator("http://h/x-5gmag-service-announcements/v1/user-data-ingest-session/"), "a locator without a session gives none");
    check(!bundleurl::mbs1UrlFromObjectLocator("http://h/x-5gmag-service-announcements/v1/user-data-ingest-session/a/b"), "a nested path gives none");
    check(!bundleurl::mbs1UrlFromObjectLocator(""), "an empty string gives none");

    /* The MBS-1 URL from the authority and the identifier alone, for a service that has no object locator (PASSED_BACK, clause 8A.2). */
    check(bundleurl::mbs1Url("127.0.0.67:8888", "abc") ==
              "http://127.0.0.67:8888/x-5gmag-service-announcements/v1/mbs-1/user-data-ingest-session/abc/",
          "the MBS-1 URL is built from the authority and the Ingest Session identifier");
    check(std::optional<std::string>(bundleurl::mbs1Url("127.0.0.67:8888", "abc")) == bundleurl::mbs1UrlFromObjectLocator(locator),
          "and is the string that the object locator gives");
    check(bundleurl::mbs1Url("[fd00::1]:8888", "9e3a") ==
              "http://[fd00::1]:8888/x-5gmag-service-announcements/v1/mbs-1/user-data-ingest-session/9e3a/",
          "an IPv6 authority is kept in it");

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
