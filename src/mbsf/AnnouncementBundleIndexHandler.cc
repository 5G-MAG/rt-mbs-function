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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <optional>
#include <string>
#include <vector>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "common.hh"
#include "ServiceAnnouncementMediaTypes.hh"
#include <DocrootHTTPRequestHandler.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>
#include "App.hh"
#include "ConditionalRequest.hh"
#include "Context.hh"
#include "MultipartMime.hh"
#include "UserDataIngSession.hh"
#include "UserServiceDiscoveryHttp.hh"
#include "hash.hh"

#include "AnnouncementBundleIndexHandler.hh"

HTTPXPP_NAMESPACE_USING(DocrootHTTPRequestHandler);
HTTPXPP_NAMESPACE_USING(HTTPResponse);
HTTPXPP_NAMESPACE_USING(HTTPServer);

MBSF_NAMESPACE_START

HTTPResponse AnnouncementBundleIndexHandler::makeResponseForDir(const std::string &dir_path, const std::string &url_path, const HTTPServer &server)
{
    return makeResponseForDir(dir_path, url_path, server, std::nullopt, std::nullopt);
}

HTTPResponse AnnouncementBundleIndexHandler::makeResponseForDir(const std::string &dir_path, const std::string &url_path, const HTTPServer &server,
                                                                const std::optional<std::string> &if_none_match,
                                                                const std::optional<std::string> &if_modified_since)
{
    if (url_path == "/" || url_path.starts_with("/.")) {
        return server.makeResponse().statusCode(403);
    }

    std::filesystem::path dp(dir_path);
    if (!dp.has_filename()) dp = dp.parent_path();
    auto user_data_ing_sess_id = dp.filename().string();

    MultipartMime bundle(MultipartMime::RELATED, USER_SERVICE_DESCRIPTIONS_MEDIA_TYPE);

    /* The entity tag is computed over the content of the parts, not over the assembled body: MultipartMime picks a fresh
       random boundary for every response, so a hash of the body would differ on every request for an unchanged bundle
       and no conditional request could match it (the same reasoning as UserServiceDiscoveryHandler). The resource was
       last modified when the newest of its parts was. */
    std::vector<char> tag_input;
    std::optional<std::chrono::system_clock::time_point> newest;

    try {
        auto user_dat_ing_session = UserDataIngSession::find(user_data_ing_sess_id);
        const auto filenames = user_dat_ing_session->getUserServiceAnnBundleFilesList();
        if (filenames.empty()) {
            return server.makeResponse().statusCode(404);
        }
        for (const auto &filename : filenames) {
            bundle.addFile(dp, filename);
            std::ifstream part(dp / filename, std::ios::binary);
            if (part) {
                tag_input.insert(tag_input.end(), filename.begin(), filename.end());
                std::vector<char> bytes((std::istreambuf_iterator<char>(part)), std::istreambuf_iterator<char>());
                tag_input.insert(tag_input.end(), bytes.begin(), bytes.end());
            }
            std::error_code ec;
            auto when = std::filesystem::last_write_time(dp / filename, ec);
            if (ec) continue;
            auto sys = std::chrono::clock_cast<std::chrono::system_clock>(when);
            if (!newest || sys > *newest) newest = sys;
        }
    } catch (std::out_of_range &ex) {
        return server.makeResponse().statusCode(404);
    }

    /* TS 26.517 V19.2.0 clause 8.2.3.4: a response of the MBSF at MBS-1 or the MBS AF at MBS-11 that carries a resource message body includes
       a strong entity tag, a modification timestamp and a Cache-Control max-age, and an endpoint that exposes GET
       supports If-None-Match and If-Modified-Since. The lifetime is the operator's userServiceDescriptionMaxAge, the
       period the MBS-5 retrieval of the same bundle states. */
    const std::string etag(std::format("\"{}\"", calculate_hash(tag_input)));
    std::optional<std::string> last_modified;
    if (newest) last_modified = httpDate(*newest);
    const std::string cache_control = discovery::cacheControlMaxAge(App::self().context()->cacheControl.UserServiceDescriptionMaxAge);

    if (getNotModified(if_none_match, if_modified_since, etag, last_modified)) {
        HTTPResponse not_modified = server.makeResponse();
        not_modified.addHeader("ETag", etag);
        not_modified.addHeader("Cache-Control", cache_control);
        if (last_modified) not_modified.addHeader("Last-Modified", *last_modified);
        not_modified.statusCode(304);
        return not_modified;
    }

    HTTPResponse resp = server.makeResponse(bundle.body());
    for (const auto &hdr : bundle.headers()) {
        resp.addHeader(hdr.first, hdr.second);
    }
    resp.addHeader("ETag", etag);
    resp.addHeader("Cache-Control", cache_control);
    if (last_modified) resp.addHeader("Last-Modified", *last_modified);

    return resp;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
