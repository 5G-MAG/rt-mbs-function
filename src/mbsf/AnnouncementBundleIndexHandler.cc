/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: AnnouncementBundleIndexHandler class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
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

#include <dirent.h>

#include <filesystem>
#include <list>
#include <string>
#include <vector>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "common.hh"
#include <DocrootHTTPRequestHandler.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>
#include "MultipartMime.hh"
#include "UserDataIngSession.hh"

#include "AnnouncementBundleIndexHandler.hh"

HTTPXPP_NAMESPACE_USING(DocrootHTTPRequestHandler);
HTTPXPP_NAMESPACE_USING(HTTPResponse);
HTTPXPP_NAMESPACE_USING(HTTPServer);

MBSF_NAMESPACE_START

HTTPResponse AnnouncementBundleIndexHandler::makeResponseForDir(const std::string &dir_path, const std::string &url_path, const HTTPServer &server)
{
    if (url_path == "/" || url_path.starts_with("/.")) {
        return server.makeResponse().statusCode(403);
    }

    std::filesystem::path dp(dir_path);
    if (!dp.has_filename()) dp = dp.parent_path();
    auto user_data_ing_sess_id = dp.filename().string();

    MultipartMime bundle(MultipartMime::RELATED);

    try {
        auto user_dat_ing_session = UserDataIngSession::find(user_data_ing_sess_id);
        auto &filenames = user_dat_ing_session->getUserServiceAnnBundleFilesList();
        if (filenames.empty()) {
            return server.makeResponse().statusCode(404);
        }
        for (const auto &filename : filenames) {
            bundle.addFile(dp, filename);
        }
    } catch (std::out_of_range &ex) {
        return server.makeResponse().statusCode(404);
    }

    HTTPResponse resp = server.makeResponse(bundle.body());
    for (const auto &hdr : bundle.headers()) {
        resp.addHeader(hdr.first, hdr.second);
    }

    return resp;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
