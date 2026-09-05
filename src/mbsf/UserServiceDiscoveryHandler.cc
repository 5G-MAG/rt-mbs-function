/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: UserServiceDiscoveryHandler class
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

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "ogs-core.h"

#include "common.hh"
#include <HTTPRequest.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>
#include "App.hh"
#include "Context.hh"
#include "MultipartMime.hh"
#include "UserDataIngSession.hh"
#include "UserService.hh"
#include "UserServiceDesc.hh"
#include "hash.hh"
#include "openapi/model/UserServiceDescriptions.h"

#include "UserServiceDiscoveryHandler.hh"

HTTPXPP_NAMESPACE_USING(HTTPRequest);
HTTPXPP_NAMESPACE_USING(HTTPResponse);
HTTPXPP_NAMESPACE_USING(HTTPServer);

using fiveg_mag_reftools::CJson;
using reftools::mbsf::UserServiceDescriptions;

MBSF_NAMESPACE_START

namespace {

// RFC 3986 section 2.1 percent-decoding, for the externalServiceId path component (cl.9.2.2
// table 9.2.2-1's own sub-resource path, "with appropriate URL encoding applied" per table
// 9.2.2-2's parallel wording for the query-parameter case). The service-class query parameter
// itself needs no separate decoding here: HTTPRequest::queryArgs() (see HTTPServer.cc's own
// MHD_GET_ARGUMENT_KIND fetch, added alongside this handler) is populated by libmicrohttpd's own
// connection-value parser, which already percent-decodes GET argument values before a handler
// ever sees them -- confirmed live, not assumed (see this change's own commit message).
std::string PercentDecode(const std::string &value)
{
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); i++) {
        if (value[i] == '%' && i + 2 < value.size() &&
            std::isxdigit(static_cast<unsigned char>(value[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(value[i + 2]))) {
            result.push_back(static_cast<char>(std::strtol(value.substr(i + 1, 2).c_str(), nullptr, 16)));
            i += 2;
        } else if (value[i] == '+') {
            result.push_back(' ');
        } else {
            result.push_back(value[i]);
        }
    }
    return result;
}

}  // namespace

HTTPResponse UserServiceDiscoveryHandler::doRequest(const HTTPRequest &request, const HTTPServer &server)
{
    if (request.method() != "GET") {
        // cl.9.2.2 table 9.2.2-1: both operations are GET-only.
        return server.makeResponse().statusCode(405);
    }

    // request.url() carries the path only by the time it reaches this handler; the query string is
    // stripped earlier (see HTTPRequest.hh).
    const std::string &path = request.url();
    // cl.9.2.2 table 9.2.2-2: both defined query parameters; either or both may be given, ANDed
    // together (table 9.2.2-1's own "resulting semantics of logical conjunction").
    std::optional<std::string> service_class = request.getQueryArgFirst("service-class");
    std::optional<std::string> profile = request.getQueryArgFirst("profile");
    if (service_class.has_value() && service_class->empty()) service_class.reset();
    if (profile.has_value() && profile->empty()) profile.reset();

    static const std::string kCollectionPath = "/user-service-descriptions";
    if (path == kCollectionPath) {
        if (!service_class.has_value() && !profile.has_value()) {
            // cl.9.2.2 table 9.2.2-1: "It is an error to invoke this operation with no query
            // parameters" -- neither of the two defined parameters was given.
            return server.makeResponse().statusCode(400);
        }
        return discover(service_class, profile, server);
    }
    if (path.starts_with(kCollectionPath + "/")) {
        std::string external_service_id = PercentDecode(path.substr(kCollectionPath.size() + 1));
        if (external_service_id.empty()) {
            return server.makeResponse().statusCode(404);
        }
        return retrieve(external_service_id, server);
    }

    return server.makeResponse().statusCode(404);
}

HTTPResponse UserServiceDiscoveryHandler::discover(const std::optional<std::string> &service_class,
                                                    const std::optional<std::string> &profile,
                                                    const HTTPServer &server) const
{
    std::vector<std::shared_ptr<UserService>> matches;
    for (const auto &[id, svc] : App::self().context()->UserServices) {
        if (!svc) continue;
        if (service_class.has_value() && svc->serviceClass() != *service_class) continue;
        if (profile.has_value() && !hasConformanceProfile(svc, *profile)) continue;
        matches.push_back(svc);
    }
    return buildBundleResponse(matches, server);
}

bool UserServiceDiscoveryHandler::hasConformanceProfile(const std::shared_ptr<UserService> &svc, const std::string &profile)
{
    for (const auto &session : svc->userDataIngSessions()) {
        if (!session || !session->isUserServiceAnnBundleAvailable()) continue;
        auto user_service_desc = session->userServiceDesc();
        if (!user_service_desc || !user_service_desc->userServiceDescription()) continue;
        for (const auto &dsd : user_service_desc->userServiceDescription()->getDistributionSessionDescriptions()) {
            if (!dsd.has_value() || !dsd.value()) continue;
            const auto &conformance_profiles = dsd.value()->getConformanceProfiles();
            if (!conformance_profiles.has_value()) continue;
            for (const auto &conf : conformance_profiles.value()) {
                if (conf.has_value() && conf.value() == profile) return true;
            }
        }
    }
    return false;
}

HTTPResponse UserServiceDiscoveryHandler::retrieve(const std::string &external_service_id, const HTTPServer &server) const
{
    std::vector<std::shared_ptr<UserService>> matches;
    for (const auto &[id, svc] : App::self().context()->UserServices) {
        if (!svc) continue;
        for (const auto &sid : svc->serviceIds()) {
            if (sid.has_value() && sid.value() == external_service_id) {
                matches.push_back(svc);
                break;
            }
        }
        if (!matches.empty()) break;  // externalServiceId is unique per cl.9.2.2 table 9.2.2-1
    }
    if (matches.empty()) {
        // cl.9.2.2 table 9.2.2-1: "Otherwise, a suitable HTTP error response code is returned."
        return server.makeResponse().statusCode(404);
    }
    return buildBundleResponse(matches, server);
}

HTTPResponse UserServiceDiscoveryHandler::buildBundleResponse(
    const std::vector<std::shared_ptr<UserService>> &matches, const HTTPServer &server) const
{
    // cl.9.2.2 table 9.2.2-1 (Discover): "...which may be empty if none match all of the
    // criteria" -- cl.9.2.3's own response table (the OpenAPI schema) makes this the 204 case.
    if (matches.empty()) {
        return server.makeResponse().statusCode(204);
    }

    UserServiceDescriptions descriptions;
    MultipartMime bundle(MultipartMime::RELATED);
    bool any_content = false;
    // Collected during the pass below, attached to `bundle` only afterward -- see the comment
    // at the bottom of this function for why the root part must be added first.
    std::vector<std::pair<std::filesystem::path, std::string>> pending_files;

    for (const auto &svc : matches) {
        // A UserService may have more than one active Ingest Session (e.g. more than one
        // Distribution Session for the same service); only the first with an
        // available announcement bundle is used to describe it, the same limitation
        // UserServiceAnnBundle's own per-session model already has for the MBS-4-MC carousel
        // path -- combining several sessions' own DistributionSessionDescriptions into a single
        // UserServiceDescription here is not attempted (no clause read so far governs it).
        for (const auto &session : svc->userDataIngSessions()) {
            if (!session || !session->isUserServiceAnnBundleAvailable()) continue;
            auto user_service_desc = session->userServiceDesc();
            if (!user_service_desc || !user_service_desc->userServiceDescription()) continue;

            descriptions.addUserServiceDescriptions(user_service_desc->userServiceDescription());
            any_content = true;

            // Attach this session's own dependent Session Description (SDP) file(s) --
            // everything getUserServiceAnnBundleFilesList() lists except announcement.json
            // itself, since that file is this session's own *singular* UserServiceDescription
            // document (used verbatim by the private x-5gmag-service-announcements path,
            // Context::createUserServAnnRequestHandler()), not the combined
            // UserServiceDescriptions root part this handler builds itself, below.
            std::filesystem::path session_dir =
                std::filesystem::path(App::self().context()->userServiceAnnDocRoot()) / session->userDataIngSessionId();
            for (const auto &filename : session->getUserServiceAnnBundleFilesList()) {
                if (filename == "announcement.json") continue;
                pending_files.emplace_back(session_dir, filename);
            }
            break;
        }
    }

    if (!any_content) {
        // Every match had no available announcement bundle yet (e.g. still being generated) --
        // honestly report "no matches ready", not a fabricated empty UserServiceDescriptions.
        return server.makeResponse().statusCode(204);
    }

    // The root part is added to `bundle` before any dependent part. RFC 2387 clause 3.1: "the
    // Content-Type field for the 'multipart/related' type contains ... optionally, a 'start'
    // parameter ... In the absence of the start parameter, ... the first body part [is] the root".
    // MultipartMime never sets a start parameter, so the first-added part is the implicit root and the
    // order is load-bearing: adding the dependent SDP files first would produce a bundle that parses
    // but names the wrong root. Hence two passes rather than one, whatever the number of matches or
    // sessions.
    CJson root_json = descriptions.toJSON(false);
    std::string root_str = root_json.serialise();
    std::vector<char> root_body(root_str.begin(), root_str.end());
    // cl.5.3.1A / clause E.2.1: MIME media type for a UserServiceDescriptions document.
    bundle.addPart(root_body, {{"Content-Type", "application/3gpp-mbs-user-service-descriptions+json"}});

    for (const auto &[session_dir, filename] : pending_files) {
        try {
            bundle.addFile(session_dir, filename);
        } catch (const std::exception &ex) {
            ogs_warn("UserServiceDiscoveryHandler: failed to attach %s in %s: %s",
                     filename.c_str(), session_dir.string().c_str(), ex.what());
        }
    }

    HTTPResponse resp = server.makeResponse(bundle.body());
    for (const auto &hdr : bundle.headers()) {
        resp.addHeader(hdr.first, hdr.second);
    }
    // cl.9.2.3.1/9.2.3.2: "A strong entity tag shall additionally be conveyed in the headers of
    // the HTTP response per clause 8.2.3.4" -- a SHA256 digest of the actual bundle body changes
    // whenever, and only when, the response representation itself changes, satisfying RFC 9111's
    // own strong-validator requirement (no W/ prefix) without needing a separate versioning
    // scheme for this dynamically-assembled document.
    resp.addHeader("ETag", std::format("\"{}\"", calculate_hash(bundle.body())));
    resp.statusCode(200);
    return resp;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
