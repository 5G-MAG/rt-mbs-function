#ifndef _MBSF_USER_SERVICE_DISCOVERY_HANDLER_HH_
#define _MBSF_USER_SERVICE_DISCOVERY_HANDLER_HH_
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

#include <memory>
#include <optional>
#include <string>

#include "common.hh"
#include <HTTPRequestHandler.hh>

HTTPXPP_NAMESPACE_START
class HTTPRequest;
class HTTPResponse;
class HTTPServer;
HTTPXPP_NAMESPACE_STOP

MBSF_NAMESPACE_START

class UserService;

// TS 26.517 V18.6.0 clause 9 (MBS AF APIs, MBS-5): "User Service Description retrieval API",
// serving this MBSF's own already-provisioned MBS User Services at reference point MBS-5, when
// MBSF is acting as (or is co-located with) the MBS AF -- the same "co-located MBS AF" HTTP
// server this app already stands up for the private x-5gmag-service-announcements path
// (Context::createUserServAnnRequestHandler()), now also serving the standard path.
//
// Clause 9.2.2: base path "{apiRoot}/3gpp-mbs-user-service-discovery/{apiVersion}/", two
// operations appended to it:
//   GET user-service-descriptions?service-class={id}&profile={id}   (Discover, either/both)
//   GET user-service-descriptions/{externalServiceId}   (Retrieve)
// Table 9.2.2-2 defines both query parameters (this pinned OpenAPI schema's own `parameters:`
// list for the Discover path carries only service-class -- a genuine mismatch between the
// prose and its own machine-readable annex, not a reason to leave profile unimplemented: the
// prose is the normative text, and DistributionSessionDescription::getConformanceProfiles()
// already exists in the generated model clause 12.3 describes).
// Registered at the versioned base path itself (see Context.cc), so doRequest() only ever sees
// the sub-resource path after that prefix has already been stripped by PathDelegatorHTTPRequestHandler.
class UserServiceDiscoveryHandler : public HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler) {
public:
    UserServiceDiscoveryHandler() {};
    UserServiceDiscoveryHandler(const UserServiceDiscoveryHandler &other) = delete;
    UserServiceDiscoveryHandler(UserServiceDiscoveryHandler &&other) = delete;

    virtual ~UserServiceDiscoveryHandler() {};

    UserServiceDiscoveryHandler &operator=(const UserServiceDiscoveryHandler &other) = delete;
    UserServiceDiscoveryHandler &operator=(UserServiceDiscoveryHandler &&other) = delete;

    virtual HTTPXPP_NAMESPACE_NAME(HTTPResponse) doRequest(const HTTPXPP_NAMESPACE_NAME(HTTPRequest) &request,
                                                            const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server);

private:
    HTTPXPP_NAMESPACE_NAME(HTTPResponse) discover(const std::optional<std::string> &service_class,
                                                   const std::optional<std::string> &profile,
                                                   const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server) const;
    // cl.9.2.2 table 9.2.2-2, "Conformance profile": true iff `svc` has at least one available
    // Ingest Session whose UserServiceDescription includes a DistributionSessionDescription
    // tagged (cl.12.3's own conformanceProfiles property) with the given profile term.
    static bool hasConformanceProfile(const std::shared_ptr<UserService> &svc, const std::string &profile);
    HTTPXPP_NAMESPACE_NAME(HTTPResponse) retrieve(const std::string &external_service_id,
                                                   const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server) const;
    // Builds the User Service Descriptions Bundle Entity (cl.9.2.3.1/9.2.3.2, cl.5.3.1A) for the
    // given already-filtered set of matching UserServices: a User Service Descriptions document
    // (cl.5.3.1A) as the root part, plus each matching service's own dependent Session
    // Description (SDP) files as additional parts. Returns 204 if `matches` is empty (cl.9.2.2.1's
    // own "may be empty if none match" case).
    HTTPXPP_NAMESPACE_NAME(HTTPResponse) buildBundleResponse(
        const std::vector<std::shared_ptr<UserService>> &matches,
        const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server) const;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_SERVICE_DISCOVERY_HANDLER_HH_ */
