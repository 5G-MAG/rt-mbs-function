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
#include "ServiceAnnouncementMediaTypes.hh"
#include <HTTPRequest.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>
#include "App.hh"
#include "Context.hh"
#include "ConditionalRequest.hh"
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
        return discover(service_class, profile, request, server);
    }
    if (path.starts_with(kCollectionPath + "/")) {
        std::string external_service_id = PercentDecode(path.substr(kCollectionPath.size() + 1));
        if (external_service_id.empty()) {
            return server.makeResponse().statusCode(404);
        }
        return retrieve(external_service_id, request, server);
    }

    return server.makeResponse().statusCode(404);
}

HTTPResponse UserServiceDiscoveryHandler::discover(const std::optional<std::string> &service_class,
                                                    const std::optional<std::string> &profile,
                                                    const HTTPRequest &request,
                                                    const HTTPServer &server) const
{
    std::vector<std::shared_ptr<UserService>> matches;
    for (const auto &[id, svc] : App::self().context()->UserServices) {
        if (!svc) continue;
        if (service_class.has_value() && svc->serviceClass() != *service_class) continue;
        if (profile.has_value() && !hasConformanceProfile(svc, *profile)) continue;
        matches.push_back(svc);
    }
    return buildBundleResponse(matches, request, server);
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

HTTPResponse UserServiceDiscoveryHandler::retrieve(const std::string &external_service_id,
                                                   const HTTPRequest &request, const HTTPServer &server) const
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
    return buildBundleResponse(matches, request, server);
}

HTTPResponse UserServiceDiscoveryHandler::buildBundleResponse(
    const std::vector<std::shared_ptr<UserService>> &matches, const HTTPRequest &request,
    const HTTPServer &server) const
{
    // cl.9.2.2 table 9.2.2-1 (Discover): "...which may be empty if none match all of the
    // criteria" -- cl.9.2.3's own response table (the OpenAPI schema) makes this the 204 case.
    if (matches.empty()) {
        return server.makeResponse().statusCode(204);
    }

    UserServiceDescriptions descriptions;
    MultipartMime bundle(MultipartMime::RELATED, USER_SERVICE_DESCRIPTIONS_MEDIA_TYPE);
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
    bundle.addPart(root_body, {{"Content-Type", USER_SERVICE_DESCRIPTIONS_MIME_TYPE}});

    /* The entity tag is computed over the content of the bundle, not over the assembled body.
       MultipartMime picks a fresh random boundary for every response, so hashing the body gave a
       different tag on every request for an unchanged resource: no conditional request could ever
       match it and no cache could ever reuse it, which is the opposite of what a validator is for.
       RFC 9110 section 8.8.1: "An entity tag is an opaque validator for differentiating between
       multiple representations of the same resource". Concatenating the parts in the order they
       are added keeps the tag strong and stable. */
    std::vector<char> tag_input(root_body);

    for (const auto &[session_dir, filename] : pending_files) {
        try {
            bundle.addFile(session_dir, filename);
            std::ifstream part(session_dir / filename, std::ios::binary);
            if (part) {
                tag_input.insert(tag_input.end(), filename.begin(), filename.end());
                std::vector<char> bytes((std::istreambuf_iterator<char>(part)),
                                         std::istreambuf_iterator<char>());
                tag_input.insert(tag_input.end(), bytes.begin(), bytes.end());
            }
        } catch (const std::exception &ex) {
            ogs_warn("UserServiceDiscoveryHandler: failed to attach %s in %s: %s",
                     filename.c_str(), session_dir.string().c_str(), ex.what());
        }
    }

    // cl.9.2.3.1/9.2.3.2: "A strong entity tag shall additionally be conveyed in the headers of
    // the HTTP response per clause 8.2.3.4" -- a SHA256 digest of the actual bundle body changes
    // whenever, and only when, the response representation itself changes, satisfying RFC 9111's
    // own strong-validator requirement (no W/ prefix) without needing a separate versioning
    // scheme for this dynamically-assembled document.
    std::string etag(std::format("\"{}\"", calculate_hash(tag_input)));

    /* TS 26.517 V18.6.0 clause 8.2.1.4: \u201cAll responses from the MBS AF that carry a message body shall include a strong entity tag in the form of an ETag response header field and a modification timestamp in the form of a Last-Modified response header per section 8.8 of RFC 9110 [19].\u201d

       The bundle is assembled per request from the files of the matching sessions, so its
       modification time is the newest of those, which is what changes when any part of the
       representation changes. */
    std::optional<std::chrono::system_clock::time_point> newest;
    for (const auto &[session_dir, filename] : pending_files) {
        std::error_code ec;
        auto when = std::filesystem::last_write_time(session_dir / filename, ec);
        if (ec) continue;
        auto sys = std::chrono::clock_cast<std::chrono::system_clock>(when);
        if (!newest || sys > *newest) newest = sys;
    }
    std::optional<std::string> last_modified;
    if (newest) {
        last_modified = std::format("{:%a, %d %b %Y %H:%M:%S GMT}",
                                    std::chrono::floor<std::chrono::seconds>(*newest));
    }

    /* TS 26.517 V18.6.0 clause 8.2.1.4: \u201cAll endpoints exposed by the MBS AF shall support conditional HTTP requests using the header fields If-none-Match and If-Modified-Since per section 13 of RFC 9110 [19].\u201d

       RFC 9110 section 13.1.1 evaluates If-None-Match ahead of If-Modified-Since and makes the
       latter moot when the former is present, so the entity tag decides whenever the client
       offered one. A 304 carries the validators and no body, per section 15.4.5. */
    bool not_modified = false;
    auto if_none_match = request.getHeaderFirst("If-None-Match");
    if (if_none_match) {
        not_modified = entityTagListMatches(*if_none_match, etag);
    } else if (last_modified) {
        auto if_modified_since = request.getHeaderFirst("If-Modified-Since");
        if (if_modified_since && notModifiedSince(*last_modified, *if_modified_since)) {
            not_modified = true;
        }
    }

    if (not_modified) {
        HTTPResponse nm = server.makeResponse();
        nm.addHeader("ETag", etag);
        if (last_modified) nm.addHeader("Last-Modified", *last_modified);
        nm.statusCode(304);
        return nm;
    }

    HTTPResponse resp = server.makeResponse(bundle.body());
    for (const auto &hdr : bundle.headers()) {
        resp.addHeader(hdr.first, hdr.second);
    }
    resp.addHeader("ETag", etag);
    if (last_modified) resp.addHeader("Last-Modified", *last_modified);
    resp.statusCode(200);
    return resp;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
