/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: announcement bundle URL unit test
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

/* Covers which URL the bundle handler answers itself (TS 26.517 V19.2.0 clause 8.2.3.4): the directory URL of one Ingest
 * Session and nothing else, so that every other path stays with the docroot handler. The answer itself needs a
 * provisioned Ingest Session and the MBSF application context and is exercised against a running MBSF.
 */

#include <cstdio>
#include <optional>
#include <string>

#include "AnnouncementBundleRequestHandler.hh"

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

static std::optional<std::string> id(const std::string &url) { return AnnouncementBundleRequestHandler::bundleSessionId(url); }

int main(int argc, char *argv[])
{
    check(id("/1/") == std::optional<std::string>("1"), "the directory URL of a session names the session");
    check(id("/9e3a1c7e-3b6e-4d9c-8d2a-0c4f6a7b8c9d/") == std::optional<std::string>("9e3a1c7e-3b6e-4d9c-8d2a-0c4f6a7b8c9d"),
          "an identifier with hyphens is taken whole");
    check(!id("/1"), "a directory URL without the trailing slash is left to the docroot handler, which redirects it");
    check(!id("/1/announcement.sdp"), "a file of a session is left to the docroot handler");
    check(!id("/1/sub/"), "a nested directory is left to the docroot handler");
    check(!id("/"), "the root is left to the docroot handler");
    check(!id("//"), "an empty name is no session");
    check(!id(""), "an empty URL is no session");
    check(!id("/./"), "the current directory is no session");
    check(!id("/../"), "the parent directory is no session");
    check(!id("/.hidden/"), "a name that starts with a dot is no session");

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
