/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS User Data Ingest Session class
 ******************************************************************************
 * Copyright: (C)2025-2026 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 *            David Waring <david.waring2@bbc.co.uk>
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

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// C library includes
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <set.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// standard template library includes
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <iostream>
#include <list>

// App header includes
#include "common.hh"
#include "ActivePeriods.hh"
#include "ActivePeriodsBase.hh"
#include "ActivePeriodsRepRule.hh"
#include "App.hh"
#include "Context.hh"
#include "CarouselObject.hh"
#include "DistributionSessionInfo.hh"
#include "hash.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Nmb2Build.hh"
#include "ObjManifest.hh"
#include "Open5GSEvent.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBINFInstance.hh"
#include "Open5GSSBIObject.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIClient.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSNetworkFunction.hh"
#include "ServiceScheduleDesc.hh"
#include <SockAddr.hh>
#include "TimerFunc.hh"
#include "utilities.hh"
#include "UserDataIngStatSubsc.hh"
#include "UserService.hh"
#include "UserServiceAnnBundle.hh"
#include "UserServiceAnnChannel.hh"
#include "UniqueMBSSessionId.hh"
#include "UserDataIngSessionNotificationEvent.hh"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/DistSession.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/Ipv6Addr.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/ObjectDistrMethInfo.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/PlmnId.h"
#include "openapi/model/ProblemCause.hh"
#include "openapi/model/ServiceScheduleDescription.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/TimeWindow.h"
#include "openapi/model/Tmgi.h"
#include "openapi/model/TunnelAddress.h"

#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"
#include "mb-smf-service-consumer.h"

// Header include for this class
#include "UserDataIngSession.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using fiveg_mag_reftools::ProblemCause;
using reftools::mbsf::AssociatedSessionId;
using reftools::mbsf::DistributionMethod;
using reftools::mbsf::DistSession;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::IpAddr;
using reftools::mbsf::Ipv6Addr;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsServiceType;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MbStfIngestAddr;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjDistributionData;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjectDistrMethInfo;
using reftools::mbsf::PacketDistrMethInfo;
using reftools::mbsf::PktDistributionOperatingMode;
using reftools::mbsf::PktIngestMethod;
using reftools::mbsf::PlmnId;
using reftools::mbsf::RepetitionRule;
using reftools::mbsf::Ssm;
using reftools::mbsf::TimeWindow;
using reftools::mbsf::Tmgi;
using reftools::mbsf::TunnelAddress;
using reftools::mbsf::ServiceScheduleDescription;

HTTPXPP_NAMESPACE_USING(SockAddr);

MBSF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbsf_userdataingsession_api_metadata(
    NMBSF_MBS_UD_INGEST_API_NAME,
    NMBSF_MBS_UD_INGEST_API_VERSION
);

using ActPeriodsType = MBSUserDataIngSession::ActPeriodsType;
using ActPeriodsRepRuleType = MBSUserDataIngSession::ActPeriodsRepRuleType;

static bool resolve_src_dest_addr(const std::string &src_ipv4_addr, const std::string &dest_ipv4_addr, struct addrinfo **ai_src, struct addrinfo **ai_dest);
static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo,
                                             void **src_addr, void **dest_addr);
static void process_mbs_distribution_session_info(const std::shared_ptr<UserDataIngSession::ContextData> &context_data,
                                                  const std::shared_ptr<DistSession> &dist_session);
static std::string print_mbs_session_error(const std::shared_ptr<UserDataIngSession::ContextData> &context_data);
static void handle_failed_mbstf_nf_instance_discover(ogs_sbi_xact_t *xact);
/* TS 29.500 V18.10.0 cl.5.2.7.2/table 5.2.7.1-1: 413 (Payload Too Large), mandatory for PATCH and
 * POST; see UserService.cc's own copy of this helper for the full citation and the residual
 * shared-framework gap it does not close. */
static bool request_too_large(Open5GSSBIRequest &request, Open5GSSBIStream &stream, int path_segments,
                              Open5GSSBIMessage &message, const NfServer::AppMetadata &app_meta,
                              const std::optional<NfServer::InterfaceMetadata> &api);
/* TS 29.500 V18.10.0 cl.6.6.2 (feature negotiation); see this file's own copy of this helper,
 * further down, for the full citation and this API's own feature table. */
static std::optional<std::string> negotiate_supp_feat(const std::optional<std::string> &requested);
static bool validate_state_setting_options(const std::shared_ptr<UserDataIngSession> &user_data_ing_session,
                                           Open5GSSBIStream &stream, Open5GSSBIMessage &message,
                                           const NfServer::AppMetadata &app_meta,
                                           const std::optional<NfServer::InterfaceMetadata> &api);
static void send_invalid_user_data_ing_session_err(const std::out_of_range &e, Open5GSSBIStream &stream,
                                                   size_t number_of_components, const Open5GSSBIMessage &message,
                                                   const NfServer::AppMetadata &app_meta,
                                                   const std::optional<NfServer::InterfaceMetadata> &api,
                                                   const std::string &user_data_ing_session_id);
static std::shared_ptr<MBSMFMBSSession> populate_mb_smf_mbs_session(
                                                        const std::shared_ptr<UserDataIngSession::ContextData> &context_data,
                                                        const std::shared_ptr<MBSMFMBSSession> &mb_smf_mbs_session);
static int64_t duration_timer(const std::chrono::system_clock::time_point &tp);
static uint64_t get_next_tsi();
static void send_model_error(const ModelException &err, Open5GSSBIStream &stream, int path_segments, Open5GSSBIMessage &message,
                             const NfServer::AppMetadata &app_meta, const std::optional<NfServer::InterfaceMetadata> &api,
                             const std::string &no_cause_reason, const std::string &log_prefix);
static void log_missing_ing_session(const std::string &id);

static std::atomic<std::uint64_t> g_next_tsi = 2;

std::recursive_mutex UserDataIngSession::s_registry_mutex;
std::map<ogs_sbi_xact_t *, std::shared_ptr<UserDataIngSession::UserDataIngDistSessId>> UserDataIngSession::s_xactRegistry;
std::map<std::string, std::shared_ptr<UserDataIngSession::UserDataIngDistSessId>> UserDataIngSession::s_distSessionIdRegistry;

UserDataIngSession::UserDataIngSession(CJson &json, bool as_request)
    :std::enable_shared_from_this<UserDataIngSession>()
    ,m_MBSUserDataIngSession(new MBSUserDataIngSession(json, as_request))
    ,m_distSessInfosMutex(new decltype(m_distSessInfosMutex)::element_type)
    ,m_deleteRequestsMutex(new decltype(m_deleteRequestsMutex)::element_type)
    ,m_serviceScheduleDescMutex(new decltype(m_serviceScheduleDescMutex)::element_type)
    ,m_sbiObject(new Open5GSSBIObject)
    ,m_generated()
    ,m_lastUsed()
    ,m_hash()
    ,m_UserDataIngSessionId()
    ,m_activePeriods(nullptr)
    ,m_activePeriodsTimer(nullptr)
    ,m_startTimer(false)
    ,m_serviceScheduleDescriptionVersion(1)
    ,m_userServiceAnnBundle(nullptr)
    ,m_carouselObjectMutex(new decltype(m_carouselObjectMutex)::element_type)
    ,m_carouselObject()
    ,m_userServiceAnnBundleAvailable(false)
    ,m_includedInCarouselObjectManifest(false)
    ,m_userSerAdNotificationSent(false)
    ,m_distributionSessionInfos()
    ,m_deleteRequests()
{
    ogs_uuid_t uuid;

    char id[OGS_UUID_FORMATTED_LENGTH + 1];

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

    std::string json_str(json.serialise());
    m_hash = calculate_hash(std::vector<std::string::value_type>(json_str.begin(), json_str.end()));

    m_UserDataIngSessionId = std::string(id);
}



UserDataIngSession::UserDataIngSession(const std::string &user_data_ing_session_id, const std::string &mbs_user_service_id,
                            const std::map<std::string, std::shared_ptr<DistributionSessionInfo>> &distribution_session_infos)
    :std::enable_shared_from_this<UserDataIngSession>()
    ,m_MBSUserDataIngSession(new MBSUserDataIngSession())
    ,m_distSessInfosMutex(new decltype(m_distSessInfosMutex)::element_type)
    ,m_deleteRequestsMutex(new decltype(m_deleteRequestsMutex)::element_type)
    ,m_serviceScheduleDescMutex(new decltype(m_serviceScheduleDescMutex)::element_type)
    ,m_sbiObject(new Open5GSSBIObject)
    ,m_generated()
    ,m_lastUsed()
    ,m_hash()
    ,m_UserDataIngSessionId()
    ,m_activePeriods(nullptr)
    ,m_activePeriodsTimer(nullptr)
    ,m_startTimer(true)
    ,m_serviceScheduleDescriptionVersion(1)
    ,m_userServiceAnnBundle(nullptr)
    ,m_carouselObjectMutex(new decltype(m_carouselObjectMutex)::element_type)
    ,m_carouselObject()
    ,m_userServiceAnnBundleAvailable(false)
    ,m_includedInCarouselObjectManifest(false)
    ,m_userSerAdNotificationSent(false)
    ,m_distributionSessionInfos()
    ,m_deleteRequests()
{
    m_MBSUserDataIngSession->setMbsUserServId(user_data_ing_session_id);

    for(const auto &[key, distribution_session_info]: distribution_session_infos) {
        const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &mbs_distribution_session_info = distribution_session_info->getMBSDistributionSessionInfo();
        m_MBSUserDataIngSession->addMbsDisSessInfos(std::string(key), std::move(mbs_distribution_session_info));
    }

    m_UserDataIngSessionId = std::string(user_data_ing_session_id);
}


UserDataIngSession::~UserDataIngSession()
{
    clearDistributionSessionInfos();
    std::lock_guard<decltype(m_deleteRequestsMutex)::element_type> lock(*m_deleteRequestsMutex);
    if (ogs_unlikely(!m_deleteRequests.empty())) {
        ogs_error("User Data Ingest Session deleted before %zu pending responses sent", m_deleteRequests.size());
    }
}

CJson UserDataIngSession::json(bool as_request = false) const
{
    return m_MBSUserDataIngSession->toJSON(as_request);
}


const std::shared_ptr<UserDataIngSession> &UserDataIngSession::find(const std::string &id)
{
    const auto &result = App::self().context()->findUserDataIngSession(id);
    if (!result) {
        throw std::out_of_range("MBS User Data Ingest session not found");
    }
    return result;
}

const std::shared_ptr<UserDataIngSession> &UserDataIngSession::locate(const std::string &id)
{
    const std::shared_ptr<UserServiceAnnChannel> ann_channel = App::self().context()->userServiceAnnouncementChannel();
    if (ann_channel) {
        const std::shared_ptr<UserDataIngSession> &ann_channel_ing_session = ann_channel->annChannelUserDataIngSession();
        if (ann_channel_ing_session->userDataIngSessionId() == id) {
            return ann_channel_ing_session;
        }
    }
    return find(id);
}

const std::shared_ptr<ServiceScheduleDesc> &UserDataIngSession::findServiceScheduleDesc(const std::string &id) const
{
    std::lock_guard<decltype(m_serviceScheduleDescMutex)::element_type> lock(*m_serviceScheduleDescMutex);
    auto it =  m_serviceScheduleDescs.find(id);
    if (it !=  m_serviceScheduleDescs.end()) {
        return it->second;
    }
    static const std::shared_ptr<ServiceScheduleDesc> null_ssd;
    return null_ssd;
}


int UserDataIngSession::numberOfDistributionSessions()
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    int count = 0;
    std::map<std::string, std::shared_ptr<UserDataIngDistSessId>>::size_type size = s_distSessionIdRegistry.size();
    if (size <= static_cast<std::map<std::string, std::shared_ptr<UserDataIngDistSessId>>::size_type>(std::numeric_limits<int>::max())) {
        count = static_cast<int>(size);
    }
    return count;
}


bool UserDataIngSession::processEvent(Open5GSEvent &event)
{

    const NfServer::InterfaceMetadata &nmbsf_mbs_userdataingsession_api = g_nmbsf_userdataingsession_api_metadata;
    const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();

    switch (event.id()) {
    case OGS_EVENT_SBI_SERVER:
        {

            Open5GSSBIRequest request(event.sbiRequest());

            Open5GSSBIMessage message;
            ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData()));
            Open5GSSBIStream stream(stream_id);

            Open5GSSBIServer server(stream.server());
            std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("Failed to parse request headers");
                return false;
            }

            //std::shared_ptr<Open5GSSBIRequest> request_ctx(new Open5GSSBIRequest(message));
            std::shared_ptr<Open5GSSBIRequest> request_ctx = nullptr;
            request_ctx.reset(new Open5GSSBIRequest(event.sbiRequest()));

            std::string service_name(message.serviceName());
            std::string resource0(message.resourceComponent(0));
            ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", service_name.c_str(), resource0.c_str());
            if (service_name == "nmbsf-mbs-ud-ingest") {
                api.emplace(nmbsf_mbs_userdataingsession_api);
            } else {
                return false;
            }

            if (api.value() == nmbsf_mbs_userdataingsession_api) {
                /******** nmbsf-mbs-ud-ingest ********/
                std::string api_version(message.apiVersion());
                if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Unsupported API version [%s]", api_version.c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta,
                                                           api, "Unsupported API version"));
                    return true;
                }

                if (resource0 == "sessions") {
                    std::string method(message.method());
                    const char *ptr_resource1 = message.resourceComponent(1);
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
                        ogs_debug("POST response: status = %i", message.resStatus());
                        std::shared_ptr<UserDataIngSession> user_data_ing_session = nullptr;
                        ogs_debug("Request body: %s", request.content());
                        if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                            return true;
                        }
                        if (request_too_large(request, stream, 3, message, app_meta, api)) return true;
                        CJson user_data_ing_sess(CJson::Null);
                        try {
                            user_data_ing_sess = CJson::parse(request.content());
                        } catch (std::exception &ex) {
                            static const char *err = "Unable to parse MBSF User Data Ingest Session as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Data Ingest Session", err));
                            return true;
                        }

                        {
                            std::string txt(user_data_ing_sess.serialise());
                            ogs_debug("Request Parsed JSON: %s", txt.c_str());
                        }

                        try {
                            user_data_ing_session.reset(new UserDataIngSession(user_data_ing_sess, true));
                        } catch (ModelException &ex) {
                            send_model_error(ex, stream, 3, message, app_meta, api, "Problem with UserDataIngSession", "Creating UserDataIngSession");
                            return true;
                        }

                        // The client's requested suppFeat is stored here so that it reaches whichever path later
                        // serialises the response: this resource completes its Create asynchronously (see
                        // processDistributionSessionInfo() below), so the negotiated value cannot be echoed from here.
                        // Feature negotiation is required by TS 29.500 cl.6.6.2.
                        {
                            const auto &mbs_user_data_ing_session = user_data_ing_session->getMBSUserIngSession();
                            mbs_user_data_ing_session->setSuppFeat(
                                    negotiate_supp_feat(mbs_user_data_ing_session->getSuppFeat()));
                        }

                        if (!validate_state_setting_options(user_data_ing_session, stream, message, app_meta, api)) return true;

                        try {
                            App::self().context()->addUserDataIngSession(user_data_ing_session);
                            //UserDataIngSession::requiresUserServiceAnnouncement(user_data_ing_session);
                            user_data_ing_session->processDistributionSessionInfo(stream_id, request_ctx);
                        } catch (std::out_of_range &ex) {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 3, message,
                                                    app_meta, api, "MBS User Service does not exist", ex.what(), std::nullopt,
                                                    std::nullopt));
                        }

                        return true;
                    } else if (method == OGS_SBI_HTTP_METHOD_GET) {
                        /* TS 29.500 V18.10.0 table 5.2.7.1-1 marks 406 mandatory for GET. This response is always
                           application/json, so a client whose Accept header cannot take that is answered 406 rather than
                           sent a body it did not ask for. */
                        std::optional<std::string> accept_hdr;
                        if (message.accept()) accept_hdr = message.accept();
                        if (!NfServer::acceptsMediaType(accept_hdr, "application/json")) {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_ACCEPTABLE, 1, message,
                                                                    app_meta, api, "Not Acceptable",
                                                                    "This resource is only available as application/json"));
                            return true;
                        }
                        if (!ptr_resource1) {
                            std::ostringstream err;
                            err << "Invalid resource [" << message.uri() << "]";
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Request", err.str()));
                            return true;
                        }
                        std::string user_data_ing_session_id(ptr_resource1);
                        try {
                            int response_code = 200;

                            std::shared_ptr<UserDataIngSession> user_data_ing_sess = find(user_data_ing_session_id);
                            CJson user_data_ing_session_json(user_data_ing_sess->json(false));
                            std::string body(user_data_ing_session_json.serialise());
                            ogs_debug("Parsed JSON: %s", body.c_str());
                            std::ostringstream location;
                            location << request.uri() << "/" << user_data_ing_sess->userDataIngSessionId();
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::string(request.uri()),
                                                    body.empty()?nullptr:"application/json",
                                                    user_data_ing_sess->generated(),
                                                    user_data_ing_sess->hash().c_str(),
                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                    std::nullopt/*nullptr*/, api, app_meta));
                            ogs_assert(response);
                            NfServer::populateResponse(response, body, response_code);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                        } catch (const std::out_of_range &e) {

                            send_invalid_user_data_ing_session_err(e, stream, 2, message, app_meta, api, user_data_ing_session_id);
                        }
                        return true;
                    } else if (method == OGS_SBI_HTTP_METHOD_PUT) {

                        if (!ptr_resource1) {
                            std::ostringstream err;
                            err << "Invalid resource [" << message.uri() << "]";
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Request", err.str()));
                            return true;
                        }
                        std::string user_data_ing_session_id(ptr_resource1);

                        if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                            return true;
                        }
                        if (request_too_large(request, stream, 3, message, app_meta, api)) return true;

                        CJson user_data_ing_sess_update(CJson::Null);
                        try {
                           user_data_ing_sess_update = CJson::parse(request.content());
                        } catch (std::exception &ex) {
                            static const char *err = "Unable to parse MBSF User Data Ingest Session update as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Data Ingest Session JSON update", err));
                            return true;
                        }

                        {
                            std::string txt(user_data_ing_sess_update.serialise());
                            ogs_debug("Patch Request Parsed JSON: %s", txt.c_str());
                        }

                        // Reject actPeriods/actPeriodsRepRule given together, mirroring the
                        // mutual-exclusion check validate_state_setting_options() already
                        // enforces on POST (TS 29.580 clause 6: the two are mutually exclusive).
                        // This is a PUT-only fix: PATCH on this resource is intentionally not
                        // implemented yet (returns 404 above), so it is not affected.
                        try {
                            MBSUserDataIngSession update_model(user_data_ing_sess_update, true);
                            if (update_model.getActPeriods() && update_model.getActPeriodsRepRule()) {
                                std::map<std::string,std::string> invalid_params;
                                invalid_params["actPeriods"] = "actPeriods cannot be present if actPeriodsRepRule is present";
                                invalid_params["actPeriodsRepRule"] = "actPeriodsRepRule cannot be present if actPeriods is present";
                                ogs_assert(true == NfServer::sendError(stream, ProblemCause::OPTIONAL_IE_INCORRECT, 3, message,
                                                                        app_meta, api, std::nullopt, std::nullopt, std::nullopt, invalid_params));
                                return true;
                            }
                        } catch (ModelException &ex) {
                            send_model_error(ex, stream, 3, message, app_meta, api, "Problem with UserDataIngSession update", "Validating UserDataIngSession update");
                            return true;
                        }

                        try {
                            std::shared_ptr<UserDataIngSession> user_data_ing_sess = find(user_data_ing_session_id);
                            user_data_ing_sess->processUserDataIngSessionUpdate(stream_id, request_ctx, user_data_ing_sess_update);
                            user_data_ing_sess->configureUserServiceAnnouncementBundler();
                            int response_code = 200;
                            CJson user_data_ing_session_json(user_data_ing_sess->json(false));
                            std::string body(user_data_ing_session_json.serialise());
                            ogs_debug("Generated JSON: %s", body.c_str());
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::string(request.uri()),
                                                    body.empty()?nullptr:"application/json",
                                                    user_data_ing_sess->generated(),
                                                    user_data_ing_sess->hash().c_str(),
                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                    std::nullopt/*nullptr*/, api, app_meta));
                            ogs_assert(response);
                            NfServer::populateResponse(response, body, response_code);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                        } catch (const std::out_of_range &e) {
                            send_invalid_user_data_ing_session_err(e, stream, 3, message, app_meta, api, user_data_ing_session_id);
                        } catch (ModelException &ex) {
                            // processUserDataIngSessionUpdate(), through updateMBSDistributionSessionInfo(), throws a
                            // ModelException for an invalid update: PATCHing objDistrInfo or pckDistrInfo while the
                            // Distribution Session is not INACTIVE is correctly rejected that way. Uncaught, the exception
                            // leaves the SBI request handler and ends the process through std::terminate(), losing every
                            // other active session over one bad client request. It is converted to an error response here,
                            // the same way the actPeriods and actPeriodsRepRule validation above is.
                            send_model_error(ex, stream, 3, message, app_meta, api, "Problem with UserDataIngSession update", "Applying UserDataIngSession update");
                        }

                        return true;

                    } else if (method == OGS_SBI_HTTP_METHOD_PATCH) {

                        // RFC 9110 clause 15.5.6: "The 405 (Method Not Allowed) status code indicates that the method
                        // received in the request-line is known by the origin server but not supported by the target
                        // resource. The origin server MUST generate an Allow header field in a 405 response containing a
                        // list of the target resource's currently supported methods." This resource exists, being the
                        // same one the GET, PUT, DELETE and OPTIONS handlers below operate on, and PATCH is a method this
                        // server recognises but this resource does not support, so 405 rather than the 404 of clause
                        // 15.5.5, which is for "the origin server did not find a current representation for the target
                        // resource". The Allow list must stay identical to the OPTIONS handler's own below.
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_METHOD_NOT_ALLOWED, 2, message,
                                                            app_meta, api, "Method not allowed",
                                                            "The PATCH method is not allowed for this path",
                                                            std::nullopt, std::nullopt, std::nullopt,
                                                            OGS_SBI_HTTP_METHOD_POST ", " OGS_SBI_HTTP_METHOD_GET ", " OGS_SBI_HTTP_METHOD_PUT ", " OGS_SBI_HTTP_METHOD_DELETE ", " OGS_SBI_HTTP_METHOD_OPTIONS));

                        return true;

                    } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
                        if (message.resourceComponent(1) && !message.resourceComponent(2)) {
                            std::string user_data_ing_session_id(message.resourceComponent(1));
                            try {
                                std::shared_ptr<UserDataIngSession> user_data_ing_sess = find(user_data_ing_session_id);
                                user_data_ing_sess->sendMbstfDelRequests();
                                //user_data_ing_sess->clearDistributionSessionInfos();
                                //std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, std::nullopt, api, app_meta));
                                //NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                //ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                                user_data_ing_sess->pendingDeleteResponse(stream_id);
                            } catch (const std::out_of_range &e) {
                                std::ostringstream err;
                                err << "MBS User Data Ingest Session [" << user_data_ing_session_id << "] does not exist.";
                                ogs_error("%s", err.str().c_str());

                                static const std::string param("{sessionId}");
                                std::ostringstream reason;
                                reason << "Invalid MBS Session identifier [" << user_data_ing_session_id << "]";
                                std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                        app_meta, api, "MBS Session not found", err.str(),
                                                        std::nullopt, invalid_params));
                            }
                            return true;
                        }
                    }  else if (method == OGS_SBI_HTTP_METHOD_OPTIONS) {
                             // Allow lists PUT but not PATCH. PUT is implemented (see the method dispatch above); the PATCH
                             // branch above returns 404 unconditionally, and the PUT handler's own comment on the actPeriods
                             // and actPeriodsRepRule check records that PATCH is deliberately not implemented on this resource
                             // yet. Advertising a method in Allow that every request 404s would invite a client to retry
                             // something that cannot succeed. Add PATCH here when it is wired up.
                             std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, OGS_SBI_HTTP_METHOD_POST ", " OGS_SBI_HTTP_METHOD_GET ", " OGS_SBI_HTTP_METHOD_PUT ", " OGS_SBI_HTTP_METHOD_DELETE ", " OGS_SBI_HTTP_METHOD_OPTIONS, api, app_meta));
                            NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                            return true;

                    } else {
                        std::ostringstream err;

                        err << "Invalid method [" << message.method() << "] for " << message.serviceName() << "/"
                                << message.apiVersion() << "/" << message.resourceComponent(0);
                        ogs_error("%s", err.str().c_str());
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                app_meta, api, "Bad request", err.str()));
                        return true;
                    }
                } else {
                    std::ostringstream err;
                    err << "Unknown object type \"" << message.resourceComponent(0) << "\" in MBSF User Data Ingest Session";
                    ogs_error("%s", err.str().c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message, app_meta,
                                                            api, "Bad request", err.str()));
                    return true;
                }
            } else {
                static const char *err = "Missing service name from URL path";
                ogs_error("%s", err);
                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta, std::nullopt,
                                                "Missing service name", err));
            }
            return true;
        }

        case MBSF_LOCAL_SEND_MBSTF_REQ_BUILD:
        {
            UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(event.sbiData());
            std::shared_ptr<UserDataIngDistSessId> ids_ptr(ids);

            try {
                //std::shared_ptr<UserDataIngSession> ing_session = find(ids_ptr->first);
                std::shared_ptr<UserDataIngSession> ing_session = locate(ids->first);
                ing_session->nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSession, new UserDataIngDistSessId(*ids), nullptr);
                return true;
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            return true;

        }
        case MBSF_LOCAL_SEND_MBSTF_PATCH_BUILD:
        {
            SessionIdContainer *ids = reinterpret_cast<SessionIdContainer*>(event.sbiData());
            try {
                //std::shared_ptr<UserDataIngSession> ing_session = find(ids->second->first);
                std::shared_ptr<UserDataIngSession> ing_session = locate(ids->second->first);
                sendMbsmfActivityStatus(ids->second);
                std::shared_ptr<UserDataIngSession::ContextData> context_data_ptr(ing_session->getDistributionSessionInfoData(ids->second->second));
                if (context_data_ptr->needsUpdate || (context_data_ptr->stateUpdate &&
                        context_data_ptr->last_reported_state !=
                                ing_session->getDistSessionState(context_data_ptr->info->getMbsDistSessState())
                        )
                   ) {
                    ing_session->nmbstfDiscoverAndSend(ids->second, Nmb2Build::buildNmb2DistSessionPatch, nullptr, ids);
                }
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->second->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            if (ids) delete ids;
            return true;

        }

        case MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION:
        {
            SessionIdContainer *ids = reinterpret_cast<SessionIdContainer*>(event.sbiData());
            try {
                //std::shared_ptr<UserDataIngSession> ing_session = find(ids->second->first);
                std::shared_ptr<UserDataIngSession> ing_session = locate(ids->second->first);
                std::shared_ptr<ContextData> context_data = ing_session->getDistributionSessionInfoData(ids->second->second);
                context_data->MBSSession->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_INACTIVE);
                context_data->MBSSession->pushChanges();
                ing_session->nmbstfDiscoverAndSend(ids->second, Nmb2Build::buildNmb2DistSessionDelete, nullptr, ids);
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->second->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            if (ids) delete ids;
            return true;
        }
        case MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK:
        {
            SessionIdContainer *ids = reinterpret_cast<SessionIdContainer*>(event.sbiData());
            try {
                std::shared_ptr<UserDataIngSession> ing_session = find(ids->second->first);

                std::shared_ptr<UserDataIngSession::ContextData> context_data_ptr(ing_session->getDistributionSessionInfoData(ids->second->second));
                if (context_data_ptr->needsUpdate || (context_data_ptr->stateUpdate &&
                        context_data_ptr->last_reported_state !=
                                ing_session->getDistSessionState(context_data_ptr->info->getMbsDistSessState()))) {
                    ing_session->nmbstfDiscoverAndSend(ids->second, Nmb2Build::buildNmb2DistSessionPatch, nullptr, ids);
                }
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->second->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            if (ids) delete ids;
            return true;
        }

        default:
            return false;
    }
    return false;
}

ogs_sbi_xact_t *UserDataIngSession::nmbstfDiscoverOnly(const std::shared_ptr<ContextData> &data)
{
    ogs_sbi_xact_t *xact = nullptr;
    ogs_sbi_discovery_option_t *discovery_option = nullptr;

    if (!data->mbstfNFInstanceId.empty()) {
        discovery_option = ogs_sbi_discovery_option_new();
        ogs_assert(discovery_option);
        ogs_sbi_discovery_option_set_target_nf_instance_id(discovery_option, const_cast<char*>(data->mbstfNFInstanceId.c_str()));
    }


    xact = m_sbiObject->discoverOnly(data->streamId, OGS_SBI_SERVICE_TYPE_NMBSTF_DISTSESSION, discovery_option);
    if (!xact) {
        ogs_error("discoverOnly() failed");
    } else {
       std::shared_ptr<UserDataIngDistSessId> ids(new UserDataIngDistSessId(data->ingSessionId, data->distSessionInfoKey));
       addToRegistry(xact, ids);
    }
    return xact;
}

ogs_sbi_xact_t *UserDataIngSession::nmbstfDiscoverAndSend(const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids,
                                                          ogs_sbi_build_f build, void *context, void *data)
{
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> user_data_ing_dist_sess_id(ids);
    ogs_sbi_xact_t *xact = nullptr;
    ogs_sbi_discovery_option_t *discovery_option = nullptr;

    std::shared_ptr<ContextData> context_data = getContextData(ids);
    if (!context_data->mbstfNFInstanceId.empty()) {
        discovery_option = ogs_sbi_discovery_option_new();
        ogs_assert(discovery_option);
        ogs_sbi_discovery_option_set_target_nf_instance_id(discovery_option,
                                                           const_cast<char*>(context_data->mbstfNFInstanceId.c_str()));
    }
    xact = m_sbiObject->discoverAndSend(context_data->streamId, OGS_SBI_SERVICE_TYPE_NMBSTF_DISTSESSION, discovery_option, build, context, data);
    if (!xact) {
        ogs_error("discoverAndSend() failed");
    } else {
       addToRegistry(xact, user_data_ing_dist_sess_id);
    }
    return xact;
}

UserDataIngSession &UserDataIngSession::setNFInstance(ogs_sbi_service_type_e service_type, ogs_sbi_nf_instance_t *nf_instance)
{
    m_sbiObject->setNFInstance(service_type, nf_instance);
    return *this;
}

UserDataIngSession &UserDataIngSession::createTimer() {
   if (!m_activePeriodsTimer) {
       m_activePeriodsTimer.reset(new Open5GSTimer(ogs_timer_add(ogs_app()->timer_mgr, changeDistSessionState, (void *)m_UserDataIngSessionId.c_str())));
       if (!m_activePeriodsTimer) {
           ogs_error("ogs_timer_add() failed");
       }
   }
   return *this;
}

const DistSessionState &UserDataIngSession::getDistSessionState(const std::optional<std::shared_ptr<DistSessionState>> &user_state) const
{
    if(m_activePeriods) {
        return m_activePeriods->currentState(user_state);
    }
    std::shared_ptr< DistSessionState > dist_sess_state = user_state.value();
    if(!dist_sess_state) {
        throw std::runtime_error("Distribution Session State is null");
    }
    return *dist_sess_state;
}

void UserDataIngSession::sendMbsmfActivityStatus(
                                    const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &user_data_ing_dist_sess_ids)
{
    std::shared_ptr<ContextData> context_data = getContextData(user_data_ing_dist_sess_ids);

    std::optional<std::shared_ptr<DistSessionState>> dist_session_state = context_data->info->getMbsDistSessState();
    if (!dist_session_state.has_value()) return;

    std::shared_ptr< DistSessionState > dist_sess_state = dist_session_state.value();

    if (( *dist_sess_state == DistSessionState::VAL_ACTIVE ) ||
                *dist_sess_state == DistSessionState::VAL_ESTABLISHED) {
           context_data->MBSSession->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_ACTIVE);
        } else {
            context_data->MBSSession->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_INACTIVE);
        }
    context_data->MBSSession->pushChanges();

}

void UserDataIngSession::sendNotificationsEvent(
                                    const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &user_data_ing_dist_sess_ids)
{
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getContextData(user_data_ing_dist_sess_ids);

    std::optional<std::shared_ptr<DistSessionState>> dist_session_state = context_data->info->getMbsDistSessState();
    if (!dist_session_state.has_value()) return;

    std::shared_ptr< DistSessionState > dist_sess_state = dist_session_state.value();

    if (*dist_sess_state == DistSessionState::VAL_ESTABLISHED) {
        try {
            std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(user_data_ing_dist_sess_ids->first);
            ing_session->pushNotificationsEvent();
        } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << user_data_ing_dist_sess_ids->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
        }
    }
}

void UserDataIngSession::pushNotificationsEvent() const
{
    std::shared_ptr<Open5GSEvent> event(new UserDataIngSessionNotificationEvent(*this));
    App::self().ogsApp()->pushEvent(event);
}

bool UserDataIngSession::startTimer()
{

    if (m_startTimer) {
        if (m_activePeriodsTimer) m_activePeriodsTimer->stop();
        m_startTimer = false;
    }

    ActivePeriodsBase::TimestampAndActiveFlag transition = m_activePeriods->nextTransition();

    if (transition.first.has_value()) {
        int64_t dur_ms = duration_timer(transition.first.value());
        m_activePeriodsTimer->start(dur_ms);
        ogs_debug("Next activePeriods event in %" PRIi64 "ms", dur_ms);
        m_startTimer = true;
        return true;
    }
    return false;
}

std::optional<SubscribedEvents::DateTime> UserDataIngSession::timeOfLatestDistributionSessionEvent(SubscribedEvents::EventTypeBitMask event_type) const
{
    std::optional<SubscribedEvents::DateTime> result;
    forEachDistributionSessionInfo([event_type, &result](const auto &id, const auto &context) -> bool {
        const SubscribedEvents &subscribed_events = context->distributionSessionInfo->eventTimestamps();
        auto timepoint = subscribed_events.timepointForEventType(event_type).first;
        if (timepoint && (!result || timepoint.value() > result.value())) {
            result = timepoint;
        }
        return true;
    });
    return result;
}

void UserDataIngSession::forEachDistributionSessionInfo(const std::function<bool(const std::string &id, const std::shared_ptr<ContextData> &ctx)> &fn)
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &[id, ctx] : m_distributionSessionInfos) {
        if (!fn(id, ctx)) break;
    }
}

void UserDataIngSession::forEachDistributionSessionInfo(const std::function<bool(const std::string &id, const std::shared_ptr<const ContextData> &ctx)> &fn) const
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &[id, ctx] : m_distributionSessionInfos) {
        if (!fn(id, std::const_pointer_cast<const ContextData>(ctx))) break;
    }
}

void UserDataIngSession::forEachServiceScheduleDesc(const std::function<bool(const std::string&, const std::shared_ptr<ServiceScheduleDesc>&)> &fn)
{
    std::lock_guard<decltype(m_serviceScheduleDescMutex)::element_type> lock(*m_serviceScheduleDescMutex);
    for (const auto &[id, desc] : m_serviceScheduleDescs) {
        if (!fn(id, desc)) break;
    }
}

void UserDataIngSession::forEachServiceScheduleDesc(const std::function<bool(const std::string&, const std::shared_ptr<const ServiceScheduleDesc>&)> &fn) const
{
    std::lock_guard<decltype(m_serviceScheduleDescMutex)::element_type> lock(*m_serviceScheduleDescMutex);
    for (const auto &[id, desc] : m_serviceScheduleDescs) {
        if (!fn(id, std::const_pointer_cast<const ServiceScheduleDesc>(desc))) break;
    }
}

void UserDataIngSession::addToDistributionSessionInfos(const std::string &key, const std::shared_ptr<ContextData> &context)
{
    auto app_context = App::self().context();
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    if (context && context->info) {
        auto &mbs_session_id = context->info->getMbsSessionId();
        if (mbs_session_id) {
            const auto &mbs_svc_area = context->info->getTgtServAreas();
            const auto &ext_mbs_svc_area = context->info->getExtTgtServAreas();
            app_context->addMbsSessionId(!!mbs_session_id.value()->getSsm(), mbs_session_id.value(),
                                         mbs_svc_area?mbs_svc_area.value():std::shared_ptr<MbsServiceArea>(),
                                         ext_mbs_svc_area?ext_mbs_svc_area.value():std::shared_ptr<ExternalMbsServiceArea>());
        }
    }
    m_distributionSessionInfos[key] = context;

}

std::shared_ptr<UserDataIngSession::ContextData> UserDataIngSession::getDistributionSessionInfoData(const std::string &key) const
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    auto it = m_distributionSessionInfos.find(key);
    if (it != m_distributionSessionInfos.end()) {
        return it->second;
    }
    return nullptr;
}

void UserDataIngSession::removeDistributionSessionInfo(const std::string &key)
{
    auto app_context = App::self().context();
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data && context_data->MBSSession) {
        context_data->MBSSession->deleteSession();
    }
    if (context_data && context_data->distributionSessionInfo) {
        auto mbs_session_id = context_data->distributionSessionInfo->getUniqueMbsSessionId();
        if (mbs_session_id && app_context->haveMbsSessionId(mbs_session_id)) {
            app_context->deleteMbsSessionId(mbs_session_id);
        }
    }
    m_distributionSessionInfos.erase(key);
}

void UserDataIngSession::deleteDistributionSessionInfo(const std::string &key)
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    m_distributionSessionInfos.erase(key);
}

void UserDataIngSession::addToRegistry(ogs_sbi_xact_t* xact, const std::shared_ptr<UserDataIngDistSessId> &ids)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    s_xactRegistry[xact] = ids;

}

void UserDataIngSession::addToRegistry(const std::string &dist_session_id, const std::shared_ptr<UserDataIngDistSessId> &ids)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    s_distSessionIdRegistry[dist_session_id] = ids;

}

void UserDataIngSession::removeFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    s_xactRegistry.erase(xact);
}

void UserDataIngSession::removeXact(ogs_sbi_xact_t* xact)
{
    if (!xact) return;
    removeFromRegistry(xact);
    ogs_sbi_xact_remove(xact);
    xact =nullptr;
}

void UserDataIngSession::removeFromRegistry(const std::string &dist_session_id)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    s_distSessionIdRegistry.erase(dist_session_id);
}


std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> UserDataIngSession::getFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    auto it = s_xactRegistry.find(xact);
    if (it != s_xactRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> UserDataIngSession::getFromRegistry(const std::string &dist_session_id)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    auto it = s_distSessionIdRegistry.find(dist_session_id);
    if (it != s_distSessionIdRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

void UserDataIngSession::changeDistSessionState(void *data)
{
    const char *id = reinterpret_cast<const char*>(data);
    std::string user_data_ing_session_id(id);
    ogs_debug("changeDistSessionState(\"%s\")", user_data_ing_session_id.c_str());
    try {
        std::shared_ptr<UserDataIngSession> user_data_ing_sess = find(user_data_ing_session_id);

        user_data_ing_sess->_changeDistSessionState();
        user_data_ing_sess->configureUserServiceAnnouncementBundler();
    }  catch (const std::out_of_range &e) {
        ogs_error("%s", std::format("MBS User Data Ingest Session [{}] does not exist.", user_data_ing_session_id).c_str());
    }
}

void UserDataIngSession::_changeDistSessionState()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (auto &[dist_sess_id, context_data] : m_distributionSessionInfos) {
        if (!context_data->mbstfDistSessionId.empty()) { // Only deal with established MBSTF DistSessions
            const auto &dist_sess_state = context_data->info->getMbsDistSessState();
            const auto &want_dist_sess_state = m_activePeriods->currentState(dist_sess_state);
            if (want_dist_sess_state != context_data->last_requested_state) {
                std::shared_ptr<UserDataIngDistSessId> ids_ptr(new UserDataIngDistSessId{m_UserDataIngSessionId, dist_sess_id});

                context_data->stateUpdate = true;
                std::shared_ptr<DistSessionState> want_state_ptr(new DistSessionState(want_dist_sess_state));
                context_data->info->setMbsDistSessState(want_state_ptr);
                context_data->distributionSessionInfo->setDistSessionState(want_state_ptr);
                sendMbsmfActivityStatus(ids_ptr);
                sendNotificationsEvent(ids_ptr);

                if (context_data->needsUpdate || want_dist_sess_state != context_data->last_reported_state) {
                    SessionIdContainer session_id{context_data->mbstfDistSessionId, ids_ptr};
                    nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSessionPatch, nullptr, &session_id);
                }
            }
        }
    }
    startTimer();
}

void UserDataIngSession::handleUserDataIngSessionUpdate(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request)
{
    setMbstfsInDesiredState();
    updateContexts(stream_id, request);
    updateMbstfRemovedDistSession();
    startTimer();
}

void UserDataIngSession::updateContexts(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request)
{
    std::shared_ptr<ContextData> ctx_data = nullptr;
    std::shared_ptr<DistSessionState> dist_sess_state = nullptr;
    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();

    for(const auto &[key, sess_info]: dist_sess_infos) {
        if (sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = sess_info.value();
            if (info) {

                std::shared_ptr<DistributionSessionInfo> distribution_session_info = nullptr;

                std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);

                if (context_data) {
                    if (context_data->needsUpdate || context_data->stateUpdate) {

                        populate_mb_smf_mbs_session(context_data, context_data->MBSSession);
                        sendLocalEventPatch(context_data->distSessionInfoKey);
                    } else {
                        continue;
                    }

                } else /*if (context_data && context_data->MBSSessionStatus == MBSSessionState::NO)*/ {

                    std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                    if (mbs_session_id.has_value()) {
                        std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                        std::optional<std::shared_ptr<Ssm> > ssm = mbs_sess_id->getSsm();
                        if (ssm.has_value()) {
                            std::shared_ptr<Ssm> ssm_val = ssm.value();
                            std::shared_ptr<IpAddr> dest_ip_addr = ssm_val->getDestIpAddr();
                            std::optional<std::string> dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
                            std::optional<std::shared_ptr<Ipv6Addr>> dest_ipv6_addr = dest_ip_addr->getIpv6Addr();
                            std::shared_ptr<Ssm> ssm_data(new Ssm(*ssm_val));
                            static std::random_device rd;
                            static std::uniform_int_distribution<in_port_t> ud(32768, 65535);
                            in_port_t port = ud(rd);
                            uint64_t tsi = 0;
                            if (info->getDistrMethod()->getValue() == DistributionMethod::VAL_OBJECT) {
                                tsi = get_next_tsi();
                            }

                            if (dest_ipv4_addr.has_value() || dest_ipv6_addr.has_value()) {
                                distribution_session_info.reset(new DistributionSessionInfo(info));
                                ctx_data.reset(new ContextData{
                                        .ingSessionId = m_UserDataIngSessionId,
                                        .distSessionInfoKey = key,
                                        .distributionSessionInfo = distribution_session_info,
                                        .info = info,
                                        .ssm = ssm_data,
                                        .ssm_port = port,
                                        .request = request,
                                        .streamId = stream_id,
                                        .tsi = tsi,
                                        // See createMbsSession()'s comment: captured here (an
                                        // instance method, has "this") rather than looked up later.
                                        .userServType = mbsUserService() ? mbsUserService()->getMBSUserServiceType() : std::string{}
                                });
                                addToDistributionSessionInfos(key, ctx_data);
                                createMbsSession(ctx_data);

                            } else {
                                ogs_error("Unable to resolve SSM addresses");
                                continue;
                            }
                        } else {
                            /* A TMGI-only mbsSessionId, carrying no ssm, is accepted. TS29571_CommonData.yaml V18.12.0's
                               MbsSessionId is anyOf[required: [tmgi], required: [ssm]], permitting either alone, and
                               TS 29.580 V18.8.0 cl.5.3.2.2.2 requires no ssm for a session identified by tmgi. That is the
                               shape a genuine Broadcast session carries, SSM being a Multicast-only concept (TS 23.247).
                               There is then no AF-nominated transport address to build the Distribution Session from, so ssm
                               is left null in ContextData; createMbsSession() and populate_mbstf_up_traffic_flow_info()
                               (Nmb2Build.cc) both handle a null ssm by having this MBSF nominate its own addressing
                               (Context::broadcastDistribution*, TS 26.502 V18.6.0 cl.4.5.6 and Annex B.3.1) rather than
                               deriving one from an ssm a Broadcast session does not carry. */
                            std::optional<std::shared_ptr<Tmgi> > tmgi = mbs_sess_id->getTmgi();
                            if (tmgi.has_value()) {
                                static std::random_device rd;
                                static std::uniform_int_distribution<in_port_t> ud(32768, 65535);
                                in_port_t port = ud(rd);
                                uint64_t tsi = 0;
                                if (info->getDistrMethod()->getValue() == DistributionMethod::VAL_OBJECT) {
                                    tsi = get_next_tsi();
                                }

                                distribution_session_info.reset(new DistributionSessionInfo(info));
                                ctx_data.reset(new ContextData{
                                        .ingSessionId = m_UserDataIngSessionId,
                                        .distSessionInfoKey = key,
                                        .distributionSessionInfo = distribution_session_info,
                                        .info = info,
                                        .ssm = nullptr,
                                        .ssm_port = port,
                                        .request = request,
                                        .streamId = stream_id,
                                        .afSuppliedTmgi = tmgi.value(),
                                        .tsi = tsi,
                                        .userServType = mbsUserService() ? mbsUserService()->getMBSUserServiceType() : std::string{}
                                });
                                addToDistributionSessionInfos(key, ctx_data);
                                createMbsSession(ctx_data);
                            } else {
                                /* Neither tmgi nor ssm present: not permitted by
                                   TS29571_CommonData.yaml's MbsSessionId anyOf. Answer rather than
                                   fall through in silence: the POST handler delegates its response
                                   to this processing and returns, so falling through leaves the
                                   request with no response at all. */
                                ogs_error("MBS Distribution Session [%s] has an mbsSessionId with neither tmgi nor ssm", key.c_str());
                                Open5GSSBIStream stream(stream_id);
                                Open5GSSBIMessage message;
                                message.parseHeader(*request);
                                NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 3, message,
                                                    App::self().mbsfAppMetadata(),
                                                    g_nmbsf_userdataingsession_api_metadata,
                                                    "mbsSessionId must contain a tmgi or an ssm",
                                                    "TS29571_CommonData.yaml's MbsSessionId requires one of tmgi or ssm.");
                                return;
                            }
                        }
                    } else {
                        /* An absent mbsSessionId is accepted. TS 29.580 V18.8.0 clause 5.3.2.2.2: "if no MBS session
                           identifier is provided, i.e. the "mbsSessionId" attribute is not present, the MBSF shall later
                           request TMGI allocation as part of the creation of the corresponding MBS session at the
                           MB-SMF". No transport address is needed first: this branch's ssm stays null exactly as the
                           TMGI-only branch's does two cases above, routing through the same "empty MBSMFMBSSession, this
                           MBSF nominates its own Nmb9 address" path (createMbsSession(), Context::broadcastDistribution*).
                           createMbsSession()'s own request_tmgi condition below covers this case, and from here on nothing
                           distinguishes it from the TMGI-only one: MB-SMF allocates the TMGI, and UserDataIngSession::tmgi(),
                           the MB-SMF create-result callback, writes it into the response's mbsSessionId.tmgi as it already
                           does for that path. */
                        static std::random_device rd;
                        static std::uniform_int_distribution<in_port_t> ud(32768, 65535);
                        in_port_t port = ud(rd);
                        uint64_t tsi = 0;
                        if (info->getDistrMethod()->getValue() == DistributionMethod::VAL_OBJECT) {
                            tsi = get_next_tsi();
                        }

                        distribution_session_info.reset(new DistributionSessionInfo(info));
                        ctx_data.reset(new ContextData{
                                .ingSessionId = m_UserDataIngSessionId,
                                .distSessionInfoKey = key,
                                .distributionSessionInfo = distribution_session_info,
                                .info = info,
                                .ssm = nullptr,
                                .ssm_port = port,
                                .request = request,
                                .streamId = stream_id,
                                .tsi = tsi,
                                .userServType = mbsUserService() ? mbsUserService()->getMBSUserServiceType() : std::string{}
                        });
                        addToDistributionSessionInfos(key, ctx_data);
                        createMbsSession(ctx_data);
                    }
                }
            }
        }
    }
}


void UserDataIngSession::userServiceAnnChannelDistributionSessionInfo()
{
    std::shared_ptr<ContextData> ctx_data = nullptr;
    std::shared_ptr<DistSessionState> dist_sess_state = nullptr;
    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();

    for(const auto &[key, sess_info]: dist_sess_infos) {
        if (sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = sess_info.value();
            if (info) {

                std::shared_ptr<DistributionSessionInfo> distribution_session_info = nullptr;

                std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);

                if (context_data) {
                    if(!isMBSSessionCreated(key)) {
                        createMbsSession(context_data);
                    } else if(isMBSSessionCreated(key) && !hasMbstfResponded(key)) {
                        sendMbstfRequests();
                    } else if (context_data->needsUpdate || context_data->stateUpdate) {
                        populate_mb_smf_mbs_session(context_data, context_data->MBSSession);
                        sendLocalEventPatch(context_data->distSessionInfoKey);
                    } else {
                        continue;
                    }

                } else {

                    std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                    if (mbs_session_id.has_value()) {
                        std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                        std::optional<std::shared_ptr<Ssm> > ssm = mbs_sess_id->getSsm();
                        if (ssm.has_value()) {
                            const std::shared_ptr<Ssm> &ssm_val = ssm.value();
                            const std::shared_ptr<IpAddr> &dest_ip_addr = ssm_val->getDestIpAddr();
                            const std::optional<std::string> &dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
                            const std::optional<std::shared_ptr<Ipv6Addr>> &dest_ipv6_addr = dest_ip_addr->getIpv6Addr();
                            std::shared_ptr<Ssm> ssm_data(new Ssm(*ssm_val));
                            // The Service Announcement channel uses the configured ssmPort, not a freshly drawn random port.
                            // A new random port per session is correct for the regular per-content-session branch above, but
                            // the announcement channel has to be a single well-known channel a client can bootstrap from
                            // static configuration: mbsf.yaml's userServiceAnnouncement.ssmPort and the matching
                            // mbsf_client.announcement_channel in rt-mbs-client.conf. A random port here would leave MBSTF
                            // transmitting the FLUTE carousel on an unpredictable port while a client listening on the
                            // configured one silently discarded every packet. Context::userServiceAnnSsmPort() carries the
                            // configured value.
                            in_port_t port = static_cast<in_port_t>(App::self().context()->userServiceAnnSsmPort());
                            uint64_t tsi = 0;
                            if (info->getDistrMethod()->getValue() == DistributionMethod::VAL_OBJECT) {
                                tsi = 1;
                            }

                            if (dest_ipv4_addr.has_value() || dest_ipv6_addr.has_value()) {
                                distribution_session_info.reset(new DistributionSessionInfo(info));
                                ctx_data.reset(new ContextData{
                                        .ingSessionId = m_UserDataIngSessionId,
                                        .distSessionInfoKey = key,
                                        .distributionSessionInfo = distribution_session_info,
                                        .info = info,
                                        .ssm = ssm_data,
                                        .ssm_port = port,
                                        .request = nullptr,
                                        .streamId = 0,
                                        .tsi = tsi,
                                        // This is the built-in Service Announcement carousel
                                        // channel (see this method's name) -- MBS-4-MC Service
                                        // Announcement is inherently a broadcast delivery, always.
                                        .userServType = std::string("BROADCAST")
                                });
                                addToDistributionSessionInfos(key, ctx_data);
                                nmbstfDiscoverOnly(ctx_data);
                                createMbsSession(ctx_data);

                            } else {
                                ogs_error("Unable to resolve SSM addresses");
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
}

const std::list<std::string> &UserDataIngSession::getUserServiceAnnBundleFilesList() const
{
    if (m_userServiceAnnBundle) return m_userServiceAnnBundle->filesToServe();
    static const std::list<std::string> empty;
    return empty;
}

void UserDataIngSession::processUserDataIngSessionUpdate(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request, CJson &json)
{
    std::shared_ptr< DistSessionState > dist_sess_state = nullptr;

    std::shared_ptr<MBSUserDataIngSession> mbs_user_data_ing_session(new MBSUserDataIngSession(json, true));
    const ActPeriodsType &act_periods = mbs_user_data_ing_session->getActPeriods();
    const ActPeriodsType &current_act_periods = m_MBSUserDataIngSession->getActPeriods();

    const ActPeriodsRepRuleType &act_periods_rep_rule = mbs_user_data_ing_session->getActPeriodsRepRule();

    if (current_act_periods.has_value()) {
        m_MBSUserDataIngSession->clearActPeriods();
    }
    m_MBSUserDataIngSession->setActPeriodsRepRule(std::nullopt);

    if (act_periods.has_value() && !act_periods->empty()) {
        activePeriods(act_periods);

        m_MBSUserDataIngSession->setActPeriods(std::move(act_periods));

        createTimer();
    } else if (act_periods_rep_rule.has_value()) {
        activePeriodsRepRule(act_periods_rep_rule);

        m_MBSUserDataIngSession->setActPeriodsRepRule(std::move(act_periods_rep_rule));

        createTimer();
    } else {
        alwaysActive();
    }

    auto app_context = App::self().context();
    const MBSUserDataIngSession::MbsDisSessInfosType &current_dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();
    MBSUserDataIngSession::MbsDisSessInfosType update_dist_sess_infos = mbs_user_data_ing_session->getMbsDisSessInfos();
    for(const auto &[key, sess_info] : current_dist_sess_infos) {
        if (!sess_info.has_value() || !sess_info.value()) {
            m_MBSUserDataIngSession->removeMbsDisSessInfos(key);
            continue;
        }
        const std::shared_ptr<MBSDistributionSessionInfo> &info = sess_info.value();
        std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);
        ogs_assert(context_data);
        context_data->needsUpdate = false;

        bool present_in_update = false;
        for (const auto &[key_in_update, sess_info_update]: update_dist_sess_infos) {
            if (key == key_in_update) {
                if (sess_info_update.has_value() && sess_info_update.value()) {
                    // update
                    std::shared_ptr<MBSDistributionSessionInfo> update_info = sess_info_update.value();

                    // TS 29.580 V18.8.0 clause 5.3.2.4.2: "The other attributes, except for the 'mbsSessionId', the
                    // 'mbsDistSessionId' and the 'locationDependent' attributes, which shall never be updated after
                    // being provisioned, ...".
                    //
                    // An update touching any of those three is rejected rather than silently restored from the stored
                    // value: restoring honours the clause in substance, but answers 200 or 204, leaving a client no
                    // way to learn its change was discarded. This matches how the structurally identical obligation on
                    // the sibling MBSUserService resource is enforced, TS 29.580 clause 5.2.2.4.2, "Only the 'servType'
                    // attribute shall not be updated", which UserService::update() rejects outright.
                    {
                        const auto &new_dist_sess_id = update_info->getMbsDistSessionId();
                        const auto &old_dist_sess_id = info->getMbsDistSessionId();
                        if (new_dist_sess_id != old_dist_sess_id) {
                            throw ModelException("mbsDistSessionId cannot be changed once provisioned",
                                    "MBSDistributionSessionInfo", "mbsDistSessionId",
                                    fiveg_mag_reftools::ProblemCause::MANDATORY_IE_INCORRECT);
                        }
                    }
                    {
                        const auto &new_mbs_sess_id = update_info->getMbsSessionId();
                        const auto &old_mbs_sess_id = info->getMbsSessionId();
                        bool mbs_sess_id_differs = new_mbs_sess_id.has_value() != old_mbs_sess_id.has_value() ||
                                (new_mbs_sess_id.has_value() && new_mbs_sess_id.value() != old_mbs_sess_id.value() &&
                                 *(new_mbs_sess_id.value()) != *(old_mbs_sess_id.value()));
                        if (mbs_sess_id_differs) {
                            throw ModelException("mbsSessionId cannot be changed once provisioned",
                                    "MBSDistributionSessionInfo", "mbsSessionId",
                                    fiveg_mag_reftools::ProblemCause::MANDATORY_IE_INCORRECT);
                        }
                    }
                    {
                        const auto &new_location_dependent = update_info->getLocationDependent();
                        const auto &old_location_dependent = info->getLocationDependent();
                        if (new_location_dependent != old_location_dependent) {
                            throw ModelException("locationDependent cannot be changed once provisioned",
                                    "MBSDistributionSessionInfo", "locationDependent",
                                    fiveg_mag_reftools::ProblemCause::MANDATORY_IE_INCORRECT);
                        }
                    }

                    // The three are now confirmed unchanged; restore them from the stored value
                    // regardless (defends against e.g. an mbsSessionId whose has_value() and
                    // operator== both agree but some other field the model doesn't compare
                    // differs) before the content_changed comparison below.
                    update_info->setMbsDistSessionId(info->getMbsDistSessionId());
                    update_info->setMbsSessionId(info->getMbsSessionId());
                    update_info->setLocationDependent(info->getLocationDependent());

                    // The comparison normalises mbsDistSessState out first, so a PUT that changes only the state is
                    // classified as a stateUpdate rather than a needsUpdate. mbsDistSessState is part of
                    // MBSDistributionSessionInfo::operator!=, and activate/deactivate is the common case since this
                    // API has no separate state-only endpoint, so comparing unnormalised would send every one of them
                    // down the needsUpdate branch, rebuilding and PATCHing the entire MBSTF distribution session
                    // instead of using the lightweight state-only path built for it (setDistSessionState() and
                    // buildNmb2DistSessionPatch()'s distSessionState branch). A change to anything else still counts
                    // as needsUpdate whether or not the state changed too: that path's rebuilt DistSession carries
                    // the new state with it.
                    const auto orig_update_state = update_info->getMbsDistSessState();
                    update_info->setMbsDistSessState(info->getMbsDistSessState());
                    bool content_changed = (*update_info != *info);
                    update_info->setMbsDistSessState(orig_update_state);

                    if (content_changed) {
                        context_data->needsUpdate = true;
                        context_data->distributionSessionInfo->updateMBSDistributionSessionInfo(update_info);
                    } else if (orig_update_state != info->getMbsDistSessState()) {
                        context_data->stateUpdate = true;
                        info->setMbsDistSessState(orig_update_state);
                        // buildNmb2DistSessionPatch()'s stateUpdate branch reads the wanted
                        // state off context_data->info, which is normally the same object as
                        // this loop's info -- set both explicitly rather than relying on that
                        // aliasing.
                        if (context_data->info && context_data->info != info) {
                            context_data->info->setMbsDistSessState(orig_update_state);
                        }
                    }
                }
                update_dist_sess_infos.erase(key_in_update);
                present_in_update = true;
                break;
            }
        }
        if (!present_in_update) {
            context_data->markForDeletion = true;
            if (context_data->distributionSessionInfo) {
                auto mbs_session_id = context_data->distributionSessionInfo->getUniqueMbsSessionId();
                if (mbs_session_id && app_context->haveMbsSessionId(mbs_session_id)) {
                    app_context->deleteMbsSessionId(mbs_session_id);
                }
            }
            m_MBSUserDataIngSession->removeMbsDisSessInfos(key);
        }
    }

    // What is left in update_dist_sess_infos are new entries so add them
    for (const auto &[key_in_update, sess_info_update]: update_dist_sess_infos) {
        const auto &mbs_session_id = sess_info_update.value()->getMbsSessionId();
        if (mbs_session_id) {
            const auto &mbs_svc_area = sess_info_update.value()->getTgtServAreas();
            const auto &ext_mbs_svc_area = sess_info_update.value()->getExtTgtServAreas();
            UniqueMbsSessionId cmp_mbs_session_id(!!mbs_session_id.value()->getSsm(), mbs_session_id.value(),
                                    mbs_svc_area?mbs_svc_area.value():std::shared_ptr<MbsServiceArea>(),
                                    ext_mbs_svc_area?ext_mbs_svc_area.value():std::shared_ptr<ExternalMbsServiceArea>());
            if (app_context->haveMbsSessionId(cmp_mbs_session_id)) {
                ogs_error("UserDataIngSession update adds already allocated MBS Session Id");
                Open5GSSBIStream stream(stream_id);
                Open5GSSBIMessage message;
                message.parseHeader(*request);
                NfServer::sendError(stream, MBSProblemCause::MBS_DIST_SESSION_ALREADY_CREATED, 2, message, App::self().mbsfAppMetadata(), g_nmbsf_userdataingsession_api_metadata, "Duplicate MBS Session Id", "UserDataIngSession update adds already allocated MBS Session Id");
                return;
            } else {
                app_context->addMbsSessionId(cmp_mbs_session_id);
            }
        }
        m_MBSUserDataIngSession->addMbsDisSessInfos(key_in_update, sess_info_update);
    }

    // Reset the states for each dist session
    for(const auto &[key, sess_info] : current_dist_sess_infos) {
        if (!sess_info || !sess_info.value()) continue;
        const std::shared_ptr<MBSDistributionSessionInfo> &info = sess_info.value();
        const auto &new_dist_state = m_activePeriods->currentState(info->getMbsDistSessState());
        std::shared_ptr<DistSessionState> dist_state(new DistSessionState(new_dist_state));
        info->setMbsDistSessState(dist_state);
    }

    if (mbsUserService() && mbsUserService()->isServiceAnnModePassedBack() &&
                        checkIfAllMBSDistributionSessionsEstablishedOrActive())
    {
        std::shared_ptr<UserServiceDesc> user_service_desc = userServiceDesc();
        userServiceAnnouncement(user_service_desc->userServiceDescription());
    } else {
        userServiceAnnouncement(nullptr);
    }
    handleUserDataIngSessionUpdate(stream_id, request);
}

void UserDataIngSession::processDistributionSessionInfo(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request)
{
    const ActPeriodsType &act_periods = m_MBSUserDataIngSession->getActPeriods();
    const ActPeriodsRepRuleType &act_periods_rep_rule = m_MBSUserDataIngSession->getActPeriodsRepRule();

    if (act_periods.has_value() && !act_periods->empty()) {
        activePeriods(act_periods);
        createTimer();
    } else if (act_periods_rep_rule.has_value()) {
        activePeriodsRepRule(act_periods_rep_rule);
        createTimer();
    } else {
        alwaysActive();
    }

    updateContexts(stream_id, request);

    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();
    for (const auto &[key, sess_info]: dist_sess_infos) {
        if (!sess_info || !sess_info.value()) continue;
        const auto &new_dist_state = m_activePeriods->currentState(sess_info.value()->getMbsDistSessState());
        std::shared_ptr<DistSessionState> dist_state(new DistSessionState(new_dist_state));
        sess_info.value()->setMbsDistSessState(dist_state);
    }

    startTimer();
}

bool UserDataIngSession::processDistSession(const std::shared_ptr<DistSession> &dist_session)
{
    if (!dist_session) return false;

    std::shared_ptr<ContextData> context_data = nullptr;
    std::shared_ptr<UserDataIngDistSessId> ids = nullptr;

    {
        std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
        auto it = s_distSessionIdRegistry.find(dist_session->getDistSessionId());
        if (it != s_distSessionIdRegistry.end()) {
            ids = it->second;
        }
    }
    ogs_assert(ids);

    context_data = getContextData(ids);

    process_mbs_distribution_session_info(context_data, dist_session);

    context_data->receivedMBSTFResponse = true;
    context_data->distSession = dist_session;

    try {
        //std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        std::shared_ptr<UserDataIngSession> ing_sess = locate(ids->first);

        if (ing_sess->isUserServiceAnnouncementChannel(ids->second))
        {
            const std::shared_ptr<UserServiceAnnChannel> &ann_channel = App::self().context()->userServiceAnnouncementChannel();
            if(ann_channel) ann_channel->notify();
            return true;
        }

        if (ing_sess->checkIfAllMBSTFResponsesReceived()) {
            ing_sess->sendNmbsfMbsUserDataIngestResponse(ids);
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }

    return true;
}

std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::setDistSessionId(const std::shared_ptr<UserDataIngSession::ContextData> &context_data, const std::string &dist_session_id)
{
    context_data->info->setMbsDistSessionId(std::string(dist_session_id));
    context_data->mbstfDistSessionId = std::string(dist_session_id);
    return context_data;

}

bool UserDataIngSession::checkIfAllMBSTFResponsesReceived()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->receivedMBSTFResponse) {
            return false;
        }
    }
    return true;

}

bool UserDataIngSession::checkIfAllMBSTFPatchResponsesReceived()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->receivedMBSTFPatchResponse) {
            return false;
        }
    }
    return true;

}

bool UserDataIngSession::resetReceivedMBSTFResponseFlags()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        dist_sess_info.second->receivedMBSTFResponse = false;
    }
    return true;

}

bool UserDataIngSession::checkIfAllMBSDistributionSessionsEstablished()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->distributionSessionInfo->dataIngestSessionEstablished()) {
            return false;
        }
    }
    return true;

}

bool UserDataIngSession::checkIfAllMBSDistributionSessionsTerminated()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->distributionSessionInfo->dataIngestSessionTerminated()) {
            return false;
        }
    }
    return true;

}

void UserDataIngSession::resetMBSDistributionSessionsTerminatedFlag()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        dist_sess_info.second->distributionSessionInfo->resetDataIngestSessionTerminated();
    }
}

void UserDataIngSession::resetMBSDistributionSessionsEstablishedFlag()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        dist_sess_info.second->distributionSessionInfo->resetDataIngestSessionEstablished();
    }
}

void UserDataIngSession::setMbstfsInDesiredState()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &[dist_sess_id, dist_sess_ctx] : m_distributionSessionInfos) {
        if (!dist_sess_ctx || !dist_sess_ctx->info) continue;
        const auto &current_dist_session_state = dist_sess_ctx->info->getMbsDistSessState();
        const DistSessionState &want_dist_session_state = getDistSessionState(current_dist_session_state);
        if (!current_dist_session_state || !current_dist_session_state.value() ||
            *current_dist_session_state.value() != want_dist_session_state) {
            changeDistSessionState(reinterpret_cast<void*>(const_cast<char*>(m_UserDataIngSessionId.c_str())));
            break;
        }
    }
}

void UserDataIngSession::checkDesiredState()
{
    setMbstfsInDesiredState();
    startTimer();
}

bool UserDataIngSession::checkIfAllMBSDistributionSessionsEstablishedOrActive()
{
    size_t dist_sessions = m_distributionSessionInfos.size();
    size_t number_of_established_or_active_sessions = 0;
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        const std::optional<std::shared_ptr< reftools::mbsf::DistSessionState > > &dist_session_state = dist_sess_info.second->distributionSessionInfo->distSessionState();
        if (!dist_session_state.has_value()) return false;
        std::shared_ptr< reftools::mbsf::DistSessionState > dist_sess_state = dist_session_state.value();
        if (!dist_sess_state) return false;
        if (dist_sess_state->getValue() == reftools::mbsf::DistSessionState::VAL_ESTABLISHED || dist_sess_state->getValue() == reftools::mbsf::DistSessionState::VAL_ACTIVE)
        {
            number_of_established_or_active_sessions++;
            //return true;
        }
    }
    return (number_of_established_or_active_sessions == dist_sessions);
}

UserDataIngSession &UserDataIngSession::userServiceAnnouncement(const std::shared_ptr<reftools::mbsf::UserServiceDescription> &user_service_description)
{
    if (user_service_description) {
        m_MBSUserDataIngSession->setMbsUserServiceAnmt(user_service_description);
    } else {
        m_MBSUserDataIngSession->setMbsUserServiceAnmt(std::nullopt);
    }
    return *this;
}

bool UserDataIngSession::sendNmbsfMbsUserDataIngestResponse(const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids)
{

    std::shared_ptr<ContextData> context_data = getContextData(ids);

    std::shared_ptr<Open5GSSBIRequest> request = context_data->request;
    if(!request) return false;

    Open5GSSBIMessage message;

    ogs_sbi_stream_t *ogs_stream = reinterpret_cast<ogs_sbi_stream_t*>(ogs_sbi_stream_find_by_id(context_data->streamId));
    if (!ogs_stream) return false;

    Open5GSSBIStream stream(context_data->streamId);

    const NfServer::InterfaceMetadata &nmbsf_mbs_userdataingsession_api = g_nmbsf_userdataingsession_api_metadata;
    std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

    try {
        message.parseHeader(*request);
    } catch (std::exception &ex) {
        ogs_error("Failed to parse request headers");
        return false;
    }

    std::string service_name(message.serviceName());
    ogs_debug("OGS_EVENT_SBI_SERVER: service=%s", service_name.c_str());
    if (service_name == "nmbsf-mbs-ud-ingest") {
        api.emplace(nmbsf_mbs_userdataingsession_api);
    } else {
        return false;
    }

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        if (ing_sess->mbsUserService() && ing_sess->mbsUserService()->isServiceAnnModePassedBack() &&
                        ing_sess->checkIfAllMBSDistributionSessionsEstablishedOrActive())
        {
            std::shared_ptr<UserServiceDesc> user_service_desc = userServiceDesc();

            ing_sess->userServiceAnnouncement(user_service_desc->userServiceDescription());
        } else {
            ing_sess->userServiceAnnouncement(nullptr);
        }

        // This gate must admit a VIA_MBS_5-only service as well, not only VIA_MBS_DISTRIBUTION_SESSION;
        // see UserDataIngSession::requiresUserServiceAnnouncement() for the reasoning. Gating on that
        // function alone would stop configureUserServiceAnnouncementBundler() being reached at all for
        // such a service.
        if (ing_sess->mbsUserService() && ing_sess->mbsUserService()->requiresUserServiceAnnouncementBundle() &&
                        ing_sess->checkIfAllMBSDistributionSessionsEstablishedOrActive() )
        {
            ing_sess->configureUserServiceAnnouncementBundler();

        }

        CJson user_data_ing_sess_json(ing_sess->json(false));
        std::string body(user_data_ing_sess_json.serialise());
        ogs_debug("Response Parsed JSON: %s", body.c_str());
        std::ostringstream location;
        location << request->uri() << "/" << ing_sess->userDataIngSessionId();
        std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                            body.empty()?nullptr:"application/json",
                            ing_sess->generated(),
                            ing_sess->hash().c_str(),
                            App::self().context()->cacheControl.MBSUserServiceMaxAge,
                            std::nullopt/*nullptr*/, api,  App::self().mbsfAppMetadata()));
        ogs_assert(response);
        NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_CREATED);
        ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));

        return true;
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
    return true;
}

bool UserDataIngSession::handleMbstfDiscover(ogs_sbi_nf_instance_t *nf_instance, ogs_sbi_xact_t *xact)
{
    if (!nf_instance) {
        handle_failed_mbstf_nf_instance_discover(xact);
        return false;
    }

    std::shared_ptr<UserDataIngDistSessId> ids = nullptr;
    {
        std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
        auto it = s_xactRegistry.find(xact);
        if (it != s_xactRegistry.end()) {
            ids = it->second;
        }
    }

    if (!ids) {
        return false;
    }

    std::shared_ptr<UserDataIngSession> ing_session = nullptr;
    try {
        //ing_session = find(ids->first);
        ing_session = locate(ids->first);
    } catch (const std::out_of_range &e) {
        log_missing_ing_session(ids->first);
        return false;
    }

    if(!ing_session) return false;

    if (nf_instance->t_validity)  ogs_timer_stop(nf_instance->t_validity);
    ing_session->setNFInstance(xact->service_type, nf_instance);

    const auto &context_data = getContextData(ids);
    if (!context_data) {
        ogs_error("Unable to get context data from registry");
        return false;
    }

    if (context_data->mbstfNFInstanceId.empty()) context_data->mbstfNFInstanceId = std::string(nf_instance->id);
    removeFromRegistry(xact);

    return true;
}

bool UserDataIngSession::createMbsSession(const std::shared_ptr<UserDataIngSession::ContextData> &context_data)
{
    // Returns early when a session object already exists for this context, rather than building a new
    // MBSMFMBSSession and its underlying mb_smf_sc_mbs_session_new_ipv4()/_ipv6() C object and then
    // discarding it. isMBSSessionCreated() turns true only once the MB-SMF session reaches CREATED
    // state, which never happens if MBSTF rejects the distribution session, and
    // userServiceAnnChannelDistributionSessionInfo()'s periodic "if (!isMBSSessionCreated(key))
    // createMbsSession(...)" check calls this on every workerLoop iteration. Rebuilding the C session
    // object and re-notifying MB-SMF each time would be an unbounded loop at the loop's own tick rate.
    // The caller's retry-driving state (MBSSessionStatus, receivedMBSTFResponse) is what progresses
    // this, not another rebuild.
    if (context_data->MBSSession) {
        ogs_debug("createMbsSession: MBS Session already exists for this context, not recreating");
        return true;
    }

    const auto &ssm_ptr = context_data->ssm;
    // ssm_ptr is null for a genuine Broadcast Distribution Session, which carries a TMGI-only
    // mbsSessionId and no AF-supplied SSM: TS29571_CommonData.yaml's MbsSessionId permits tmgi alone
    // and TS 29.580 cl.5.3.2.2.2 does not require ssm. Leaving all four *_addr optionals unset routes
    // a null ssm_ptr into the "!src_ipv4_addr && !src_ipv6_addr" empty-MBSMFMBSSession branch below,
    // which builds the no-SSM session correctly (mb_smf_sc_mbs_session_new(), setTunnelRequest(true)).
    // Reading ssm_ptr->getDestIpAddr() here instead would dereference null.
    std::optional<std::string> dest_ipv4_addr, src_ipv4_addr;
    std::optional<std::shared_ptr<Ipv6Addr>> dest_ipv6_addr, src_ipv6_addr;
    if (ssm_ptr) {
        const auto &dest_ip_addr = ssm_ptr->getDestIpAddr();
        dest_ipv4_addr = dest_ip_addr?dest_ip_addr->getIpv4Addr():std::nullopt;
        dest_ipv6_addr = dest_ip_addr?dest_ip_addr->getIpv6Addr():std::nullopt;
        const auto &src_ip_addr = ssm_ptr->getSourceIpAddr();
        src_ipv4_addr = src_ip_addr?src_ip_addr->getIpv4Addr():std::nullopt;
        src_ipv6_addr = src_ip_addr?src_ip_addr->getIpv6Addr():std::nullopt;
    }

    std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session = nullptr;
    if (!src_ipv4_addr && !src_ipv6_addr) {
        ogs_debug("Making empty MBSMFMBSSession");
        mb_smf_mbs_session.reset(new MBSMFMBSSession(mb_smf_sc_mbs_session_new()));
        mb_smf_mbs_session->setTunnelRequest(true);
    } else if (src_ipv4_addr && dest_ipv4_addr) {
        struct addrinfo *ai_src = NULL, *ai_dest = NULL;
        void *src_addr = NULL, *dest_addr = NULL;

        if (resolve_src_dest_addr(src_ipv4_addr.value(), dest_ipv4_addr.value(), &ai_src, &ai_dest))
        {
            if (get_src_dest_of_same_addr_family(AF_INET, ai_src, ai_dest, &src_addr, &dest_addr))
            {
                ogs_debug("Making MBSMFMBSSession: src=%s dst=%s", src_ipv4_addr.value().c_str(), dest_ipv4_addr.value().c_str());
                mb_smf_mbs_session.reset(new MBSMFMBSSession(
                            mb_smf_sc_mbs_session_new_ipv4((const struct in_addr*)src_addr, (const struct in_addr*)dest_addr)));
            } else {
               ogs_error("Unable to resolve SSM addresses for IPv4 address family");
               if (ai_src) {
                   freeaddrinfo(ai_src);
                   ai_src = NULL;
               }

               if (ai_dest) {
                   freeaddrinfo(ai_dest);
                   ai_dest = NULL;
                }
                return false;
            }
        } else {
            ogs_error("Unable to resolve SSM addresses for IPv4 address family");
            return false;
        }
        if (ai_src) {
            freeaddrinfo(ai_src);
            ai_src = NULL;
        }

        if (ai_dest) {
            freeaddrinfo(ai_dest);
            ai_dest = NULL;
        }
    } else if (src_ipv6_addr && dest_ipv6_addr) {
        struct addrinfo *ai_src = NULL, *ai_dest = NULL;
        void *src_addr = NULL, *dest_addr = NULL;

        if (resolve_src_dest_addr(*src_ipv6_addr.value(), *dest_ipv6_addr.value(), &ai_src, &ai_dest))
        {
            if (get_src_dest_of_same_addr_family(AF_INET6, ai_src, ai_dest, &src_addr, &dest_addr))
            {
                ogs_debug("Making MBSMFMBSSession: src=%s dst=%s", src_ipv6_addr.value()->c_str(), dest_ipv6_addr.value()->c_str());
                mb_smf_mbs_session.reset(new MBSMFMBSSession(
                mb_smf_sc_mbs_session_new_ipv6((const struct in6_addr*)src_addr, (const struct in6_addr*)dest_addr)));

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv6 address family");
               if (ai_src) freeaddrinfo(ai_src);
               if (ai_dest) freeaddrinfo(ai_dest);
               return false;
           }
        } else {
            ogs_error("Unable to resolve SSM addresses for IPv6 address family");
            return false;
        }
        if (ai_src) freeaddrinfo(ai_src);
        if (ai_dest) freeaddrinfo(ai_dest);

    } else {
       ogs_error("Unable to resolve SSM addresses");
       return false;
    }

    // This block runs whether or not the MBSMFMBSSession carries an SSM. None of it is SSM-specific:
    // populate_mb_smf_mbs_session() reads only context_data->info fields, and setServiceType() and
    // setCallback() are the only way the MB-SMF session just created is told its service type or
    // reports back. Gating it on ssm() would leave a genuine Broadcast (TMGI-only) Distribution
    // Session created locally but never announced to MB-SMF, with no caller notified of completion.
    // code-derived, no spec claim.
    {

        mb_smf_mbs_session->setTunnelRequest(true);
        /* TS 29.580 V18.8.0 clause 5.3.2.2.2 gives two triggers for TMGI allocation, evaluated per
           map entry of "mbsDisSessInfos":

             "if no MBS session identifier is provided, i.e. the "mbsSessionId" attribute is not
              present, the MBSF shall later request TMGI allocation as part of the creation of the
              corresponding MBS session at the MB-SMF; and"

             "if a source specific multicast address (SSM) is provided within the "mbsSessionId"
              attribute and the "locationDependent" attribute is present and set to "true" (i.e. to
              indicate a location dependent MBS service), the MBSF shall also request TMGI
              allocation as part of the creation of the corresponding MBS session at the MB-SMF."

           The second is an additional case, not a narrowing of the first: an absent mbsSessionId
           requires a TMGI whatever locationDependent says, because there is otherwise no identifier
           for the session at all.

           The test is on the provisioned mbsSessionId, not on the SSM resolved above, which is
           present here either way. */
        bool request_tmgi = false;
        if (context_data->info) {
            const std::optional<std::shared_ptr<MbsSessionId> > &mbs_session_id =
                    context_data->info->getMbsSessionId();
            if (!mbs_session_id.has_value() || !mbs_session_id.value()) {
                request_tmgi = true;
            } else {
                const std::optional<bool> &location_dependent = context_data->info->getLocationDependent();
                if (mbs_session_id.value()->getSsm().has_value() &&
                    location_dependent.has_value() && location_dependent.value()) {
                    request_tmgi = true;
                }
            }
        }
        // Three mutually-exclusive states, per the vendored mb-smf-service-consumer library's
        // own contract (mbs-session.h's own documented contract for
        // mb_smf_sc_mbs_session_set_tmgi(): setting a TMGI and the TMGI-request flag are
        // mutually exclusive): an AF-supplied TMGI (context_data->afSuppliedTmgi, parsed from
        // mbsSessionId.tmgi), a request for MB-SMF to allocate one (request_tmgi, computed
        // above), or neither. request_tmgi as computed above can never be true at the same
        // time as afSuppliedTmgi is set (it is only set when mbsSessionId is absent, or when
        // an SSM -- not a tmgi -- was supplied with locationDependent=true), so the two
        // branches are not a priority order in practice, but afSuppliedTmgi is checked first
        // to honour the library's own mutual-exclusivity rule regardless.
        if (context_data->afSuppliedTmgi) {
            mb_smf_mbs_session->setTmgi(context_data->afSuppliedTmgi);
        } else if (request_tmgi) {
            mb_smf_mbs_session->setTmgiRequest(true);
        }

        // The service type sent to the SMF and MB-SMF comes from the parent MBS User Service's own
        // servType, read through UserService::getMBSUserServiceType(), not a fixed value. SMF's Nmbsmf
        // handler (n4mb-handler.c) makes the Namf_MBSBroadcast context-create call, the step that drives
        // NGAP Broadcast Session Setup to the gNB, only "if the service type is broadcast service"
        // (TS 23.247 cl.7.3.1 step 2); for MULTICAST it correctly does nothing here, multicast UE-join
        // being a separate Namf_MBSCommunication procedure. Sending MULTICAST for a BROADCAST service
        // would therefore let PFCP/N4mb and MBSTF FLUTE transmission complete while NGAP never reached
        // the gNB, leaving no MRB for the session and content with no bearer, and no error anywhere.
        mb_smf_mbs_session->setServiceType(
            ogs_strcasecmp(context_data->userServType.c_str(), "BROADCAST") == 0
                ? MBS_SERVICE_TYPE_BROADCAST : MBS_SERVICE_TYPE_MULTICAST);
        if (!context_data->MBSSession) context_data->MBSSession = mb_smf_mbs_session;
        mb_smf_mbs_session->setCallback(UserDataIngDistSessId(context_data->ingSessionId, context_data->distSessionInfoKey));
        populate_mb_smf_mbs_session(context_data, mb_smf_mbs_session);
    }

    return true;
}

void UserDataIngSession::deleteMBSTFSession(ogs_sbi_xact_t *xact)
{
    std::shared_ptr<UserDataIngDistSessId> ids = nullptr;

    {
        std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
        auto it = s_xactRegistry.find(xact);
        if (it != s_xactRegistry.end()) {
            ids = it->second;
        }
    }

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        ing_sess->sendMbstfDelRequests();
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

bool UserDataIngSession::handlePatchUpdateResponse(ogs_sbi_xact_t *xact, const std::shared_ptr<reftools::mbsf::DistSession> &dist_session)
{
    std::shared_ptr<UserDataIngDistSessId> ids = nullptr;

     {
        std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
        auto it = s_xactRegistry.find(xact);
        if (it != s_xactRegistry.end()) {
            ids = it->second;
        }
    }

    try {
        //std::shared_ptr<UserDataIngSession> ing_session = find(ids->first);
        std::shared_ptr<UserDataIngSession> ing_session = locate(ids->first);
        std::shared_ptr<ContextData> context_data = getContextData(ids);
        context_data->receivedMBSTFPatchResponse = true;
        context_data->patchUpdateSucceded = true;
        context_data->needsUpdate = false;
        context_data->stateUpdate = false;
        context_data->distSession = dist_session;
        if (ing_session->isUserServiceAnnouncementChannel(ids->second))
        {
            const std::shared_ptr<UserServiceAnnChannel> &ann_channel = App::self().context()->userServiceAnnouncementChannel();
            if(ann_channel) ann_channel->notify();
            return true;
        }

        ing_session->checkDesiredState();

    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
    return true;

}


void UserDataIngSession::rollbackMBSTFDistSessionState(ogs_sbi_xact_t *xact)
{
    std::shared_ptr<UserDataIngDistSessId> ids = nullptr;


    {
        std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
        auto it = s_xactRegistry.find(xact);
        if (it != s_xactRegistry.end()) {
            ids = it->second;
        }
    }

    try {
        //std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        std::shared_ptr<UserDataIngSession> ing_sess = locate(ids->first);

        std::shared_ptr<ContextData> context_data = getContextData(ids);

        context_data->receivedMBSTFPatchResponse = true;
        context_data->patchUpdateSucceded = false;
        context_data->needsUpdate = false;
        ing_sess->setMbstfsInDesiredState();
        if (!context_data->stateUpdate) return;
        if (ing_sess->checkIfAllMBSTFPatchResponsesReceived()) {
            ing_sess->sendMbstfPatchRollbackRequests();
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}


std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::getContextData(const std::shared_ptr<UserDataIngDistSessId> &ids)
{
    try {
        //std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        std::shared_ptr<UserDataIngSession> ing_sess = locate(ids->first);
        return ing_sess->getDistributionSessionInfoData(ids->second);
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
    return nullptr;

}


void UserDataIngSession::removeDistributionSessionInfos(const UserDataIngDistSessId &ids)
{
    try{
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids.first);
        // Do something here?
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids.first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::clearDistributionSessionInfos()
{
    auto app_context = App::self().context();
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (auto &dist_sess_info : m_distributionSessionInfos)
    {
        std::shared_ptr<ContextData> context_data = dist_sess_info.second;
        if (context_data->MBSSession) {
            context_data->MBSSession->deleteSession();
        }
        if (app_context && context_data->distributionSessionInfo) {
            auto mbs_session_id = context_data->distributionSessionInfo->getUniqueMbsSessionId();
            if (mbs_session_id && app_context->haveMbsSessionId(mbs_session_id)) {
                app_context->deleteMbsSessionId(mbs_session_id);
            }
        }
    }

    m_distributionSessionInfos.clear();
}

bool UserDataIngSession::checkIfAllMBSSessionCreated()
{
    bool all_mbs_sessions_created = true;

    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus != MBSSessionState::CREATED) {
            all_mbs_sessions_created = false;
            break;
        }
    }
    if (all_mbs_sessions_created) {
        sendMbstfRequests();
    }
    return all_mbs_sessions_created;

}

bool UserDataIngSession::checkIfAllMBSSessionDeletionsReceived()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus != MBSSessionState::DELETED) {
            return false;
        }
    }
    return true;
}

void UserDataIngSession::sendMbstfRequests()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (auto &dist_sess_info : m_distributionSessionInfos)
    {

        std::shared_ptr<ContextData> context_data(dist_sess_info.second);
        if (context_data->receivedMBSTFResponse) continue;
        UserDataIngDistSessId *ids = new UserDataIngDistSessId(context_data->ingSessionId, context_data->distSessionInfoKey);

        sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_REQ_BUILD, ids);

    }
}

bool UserDataIngSession::checkIfAllMBSTFDistSessionDeleted()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->MBSTFDistSessionDeleted) {
            return false;
        }
    }

    std::erase_if(s_distSessionIdRegistry,
                  [this](std::pair<const std::string, std::shared_ptr< UserDataIngDistSessId >> &x) -> bool
                  {
                    return x.second->first == m_UserDataIngSessionId;
                  });

    //App::self().context()->deleteUserDataIngSession(m_UserDataIngSessionId);

    return true;
}

void UserDataIngSession::sendMbstfDelRequests(const std::optional<std::string>& key)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);

    for (auto &[dist_sess_id, user_ing_sess_id_ptr] : s_distSessionIdRegistry) {
        // match session id and, if key provided, match the key
        if (user_ing_sess_id_ptr->first == m_UserDataIngSessionId &&
            (!key.has_value() || user_ing_sess_id_ptr->second == *key))
        {
            SessionIdContainer* session_id = new SessionIdContainer(dist_sess_id, user_ing_sess_id_ptr);
            sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION, session_id);

            // if a key was provided, only process the first match
            if (key.has_value()) break;
        }
    }
}

void UserDataIngSession::sendLocalEventPatch(const std::optional<std::string>& key)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);

    for (auto &[dist_sess_id, user_ing_sess_id_ptr] : s_distSessionIdRegistry) {
        // match session id and, if key provided, match the key
        if (user_ing_sess_id_ptr->first == m_UserDataIngSessionId &&
            (!key.has_value() || user_ing_sess_id_ptr->second == *key))
        {
            SessionIdContainer* session_id = new SessionIdContainer(dist_sess_id, user_ing_sess_id_ptr);
            sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_PATCH_BUILD, session_id);

            // if a key was provided, only process the first match
            if (key.has_value())
                break;
        }
    }
}

void UserDataIngSession::updateMbstfRemovedDistSession()
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);

    for (auto &[dist_sess_id, user_ing_sess_id_ptr] : s_distSessionIdRegistry) {
        if (user_ing_sess_id_ptr->first == m_UserDataIngSessionId)
        {
            std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(user_ing_sess_id_ptr->second);
            if (context_data && context_data->markForDeletion) {
                SessionIdContainer* session_id = new SessionIdContainer(dist_sess_id, user_ing_sess_id_ptr);
                sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION, session_id);
            }
        }
    }
}

void UserDataIngSession::sendMbstfPatchRollbackRequests()
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);

    for (auto &[dist_sess_id, user_ing_sess_id_ptr] : s_distSessionIdRegistry) {
        if (user_ing_sess_id_ptr->first == m_UserDataIngSessionId) {
            SessionIdContainer* session_id = new SessionIdContainer(dist_sess_id, user_ing_sess_id_ptr);
            sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK, session_id);
        }

    }
}

void UserDataIngSession::sendLocalEvent(OgsExtendedEventId event_id, void *data)
{
    int rv;

    Open5GSEvent local_event(ogs_event_new(event_id));
    local_event.setSbiData(data);

    rv = ogs_queue_push(ogs_app()->queue, local_event.ogsEvent());
    if (rv != OGS_OK) {
        ogs_error("Failed to push MBSF local  Build MBSTF event onto the eveint queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}


const char *UserDataIngSession::localEventGetName( ogs_event_t *event)
{

    if (ogs_unlikely(!event)) return "*** No Event ***";

    switch (event->id) {
        case MBSF_LOCAL_SEND_MBSTF_REQ_BUILD:
            return "MBSF_LOCAL_SEND_MBSTF_REQ_BUILD";
        case MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK:
            return "MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK";
        default:
            return ogs_event_get_name(event);
    }

    return "Unknown MBSF LOCAL Event";
}

void UserDataIngSession::setMBSSessionFlag(const UserDataIngDistSessId &ids)
{
    try {
        std::shared_ptr<UserDataIngSession> ing_sess = locate(ids.first);
        std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids.second);
        context_data->MBSSessionStatus = MBSSessionState::CREATED;
        if (ing_sess->isUserServiceAnnouncementChannel(ids.second))
        {
            const std::shared_ptr<UserServiceAnnChannel> &ann_channel = App::self().context()->userServiceAnnouncementChannel();
            if(ann_channel) ann_channel->notify();
            return;
        }
        if (ing_sess->checkIfAllMBSSessionResponsesReceived()) {
            bool rv = ing_sess->checkIfAllMBSSessionCreated();
            if (!rv) {
                ing_sess->handleFailedMBSSession();
            }

        }
        //ing_sess->checkIfAllMBSSessionCreated();
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids.first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::setMBSSessionDeleted(const UserDataIngDistSessId &ids)
{
    try {
        // locate(), not find(): the announcement channel's own MBS User Data Ingest Session is
        // constructed directly by UserServiceAnnChannel rather than through
        // Context::addUserDataIngSession(), so find()'s lookup can never succeed for it and MB-SMF
        // deletion handling for that session would always throw "does not exist" below and do nothing.
        // setMBSSessionFlag() above uses locate() for the same reason.
        std::shared_ptr<UserDataIngSession> ing_sess = locate(ids.first);
        std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids.second);
        context_data->MBSSessionStatus = MBSSessionState::DELETED;
        if (ing_sess->checkIfAllMBSSessionDeletionsReceived()) {
            const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();
            std::lock_guard<decltype(ing_sess->m_deleteRequestsMutex)::element_type> lock(*ing_sess->m_deleteRequestsMutex);
            for (auto id : ing_sess->m_deleteRequests) {
                Open5GSSBIStream stream(id);
                std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, std::nullopt, g_nmbsf_userdataingsession_api_metadata, app_meta));
                NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
            }
            ing_sess->m_deleteRequests.clear();
            if (context_data->markForDeletion) {
                // The keys come from context_data's own fields, not from the ids pair: removeFromRegistry() is
                // keyed by the MBSTF-assigned distribution session ID while removeDistributionSessionInfo() is
                // keyed by distSessionInfoKey, and the ids pair holds them the other way round. erase() by a
                // wrong key no-ops with no exception and no log, so s_distSessionIdRegistry and
                // m_distributionSessionInfos would never be cleared, leaving the deleted session's MbsSessionId
                // and its SSM address registered in Context::m_mbsSessionIds for the life of the process; the
                // next create reusing that SSM then trips Context::addMbsSessionId's "Attempt to insert
                // duplicate" warning.
                removeFromRegistry(context_data->mbstfDistSessionId);
                ing_sess->removeDistributionSessionInfo(context_data->distSessionInfoKey);
            }
            App::self().context()->deleteUserDataIngSession(ing_sess->m_UserDataIngSessionId);
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids.first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::setMBSSessionFailureFlag(const UserDataIngDistSessId &ids, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{
    std::string ids_first(ids.first);
    try {
        // locate() rather than find(), for the reason given in setMBSSessionDeleted() above: find()
        // always throws for the announcement channel's own MBS Session, so a failed MB-SMF Create for it
        // could never be recorded and would fall through to the catch below.
        std::shared_ptr<UserDataIngSession> ing_sess = locate(ids.first);
        std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids.second);
        context_data->MBSSessionStatus = MBSSessionState::FAILED;
        //context_data->hasMBSSession = true;
        if (ing_sess->checkIfAllMBSSessionResponsesReceived()) {
            populateAndSendError(new UserDataIngDistSessId(ids), cause, problem_detail_json);
            App::self().context()->deleteUserDataIngSession(ids_first);
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids_first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::handleFailedMBSSession()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus == MBSSessionState::FAILED) {
            UserDataIngDistSessId *ids = new UserDataIngDistSessId(dist_sess_info.second->ingSessionId,
                                                                   dist_sess_info.second->distSessionInfoKey);
            populateAndSendError(ids, dist_sess_info.second->mbsmfProblemCause, dist_sess_info.second->mbsmfProblemDetailJson);
        }
    }
}


bool UserDataIngSession::checkIfAllMBSSessionResponsesReceived()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus != MBSSessionState::CREATED &&
            dist_sess_info.second->MBSSessionStatus != MBSSessionState::FAILED) {
            return false;
        }
    }
    return true;
}

void UserDataIngSession::setMBSTFDistSessionDeletedFlag(const std::string &dist_session_id)
{
    ogs_debug("Deleted Dist Session %s on MBSTF", dist_session_id.c_str());

    std::shared_ptr<UserDataIngDistSessId> ids = getFromRegistry(dist_session_id);
    std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
    std::shared_ptr<ContextData> context_data = getContextData(ids);
    if (context_data) {
        context_data->MBSTFDistSessionDeleted = true;
        if (context_data->MBSSession) {
            ogs_debug("Deleting MBS Session for Dist Session %s", dist_session_id.c_str());
            context_data->MBSSession->deleteSession();
        }
        // dist_session_id, this function's own parameter, is already the registry key; no field lookup is
        // needed. removeDistributionSessionInfo() is keyed by distSessionInfoKey rather than by the
        // ingSessionId UUID, and erase() by the wrong key no-ops silently, so passing the wrong one here
        // would leave the session in s_distSessionIdRegistry and m_distributionSessionInfos and never drop
        // the UserDataIngSession's last shared_ptr. Its MbsSessionId and SSM address would then stay in
        // Context::m_mbsSessionIds for the life of the process, and the next create reusing that SSM would
        // trip Context::addMbsSessionId's "Attempt to insert duplicate" warning.
        if (context_data->markForDeletion) {
            removeFromRegistry(dist_session_id);
            ing_sess->removeDistributionSessionInfo(context_data->distSessionInfoKey);
            return;
        }
    }
    // deleteUserDataIngSession() is deliberately not called here. setMBSSessionDeleted() is the
    // authoritative "whole ingest session torn down" trigger: it sends the deferred DELETE responses
    // queued in m_deleteRequests before destroying the session, gated on
    // checkIfAllMBSSessionDeletionsReceived(). A normal delete fires both this function, for the MBSTF
    // distribution session, and setMBSSessionDeleted(), for the MB-SMF MBS Session, against the same
    // logical session; destroying the UserDataIngSession from here would race the two and can win,
    // leaving setMBSSessionDeleted() unable to send its queued response and the client's DELETE
    // waiting for one that can no longer come. checkIfAllMBSTFDistSessionDeleted() still runs for its
    // s_distSessionIdRegistry cleanup, and omits the same call for the same reason.
    ing_sess->checkIfAllMBSTFDistSessionDeleted();
}

void UserDataIngSession::populateAndSendError(UserDataIngDistSessId *ids, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{

    std::shared_ptr<UserDataIngDistSessId> ids_ptr(ids);
    std::shared_ptr<ContextData> context_data = getContextData(ids_ptr);

    std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

    std::shared_ptr<Open5GSSBIRequest> request = context_data->request;
    Open5GSSBIStream stream;
    try {
        stream = std::move(Open5GSSBIStream(context_data->streamId));
    } catch (std::runtime_error &ex) {
        /* Stream doesn't exist, so nowhere to send the error */
        return;
    }
    Open5GSSBIServer server(stream.server());
    Open5GSSBIMessage message;

    try {
            message.parseHeader(*request);
    } catch (std::exception &ex) {
       ogs_error("Failed to parse headers");
       return;
    }

    removeDistributionSessionInfos(*ids);

    std::ostringstream err;

    // An upstream problem detail is preferred over print_mbs_session_error(), whose message is
    // hardcoded to "already exists in the MBS System". That wording fits a client's Create clashing
    // with an existing session or TMGI, but not the other failures reaching this function: an MB-SMF
    // Create timeout, or a 403 or other rejection whose detail arrived on the wire in
    // problem_detail_json. The fallback is kept for a bare timeout carrying no problem_details, the
    // one remaining case the hardcoded wording still describes.
    std::string error;
    if (problem_detail_json.has_value()) {
        CJson detail_node = problem_detail_json->getObjectItemCaseSensitive("detail");
        if (!detail_node.isNull() && detail_node.isString()) {
            error = std::string(detail_node);
        }
    }
    if (error.empty()) {
        error = print_mbs_session_error(context_data);
    }

    if (cause.has_value()) {
        ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, cause.value(), error.c_str()));

    } else {

        ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, ProblemCause::INBOUND_SERVER_ERROR, error.c_str()));

    }
}


bool UserDataIngSession::tmgi(mb_smf_sc_tmgi_t *tmgi, const UserDataIngDistSessId &ids)
{
    bool tmgi_set = false;

    //const char *tmgi_repr = mb_smf_sc_tmgi_repr(tmgi);
    //char *plmn_id = ogs_plmn_id_to_string(&tmgi->plmn, buf);
    char *mcc = ogs_plmn_id_mcc_string(&tmgi->plmn);
    char *mnc = ogs_plmn_id_mnc_string(&tmgi->plmn);

    //std::shared_ptr<UserDataIngSession> ing_sess = find(ids.first);
    std::shared_ptr<UserDataIngSession> ing_sess = locate(ids.first);
    std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids.second);
    if (context_data && context_data->info) {
        if (tmgi) context_data->tmgi = tmgi;
        std::shared_ptr<Tmgi> mgi = nullptr;
        std::shared_ptr<PlmnId> plmn_id = nullptr;

        plmn_id.reset(new PlmnId());
        plmn_id->setMcc(std::string(mcc));
        plmn_id->setMnc(std::string(mnc));

        mgi.reset(new Tmgi());
        mgi->setMbsServiceId(std::string(tmgi->mbs_service_id));
        mgi->setPlmnId(plmn_id);

        std::optional<std::shared_ptr< MbsSessionId > > mbs_sess_id = context_data->info->getMbsSessionId();
        if (mbs_sess_id.has_value()) {
            std::shared_ptr< MbsSessionId > sess_id = mbs_sess_id.value();
            sess_id->setTmgi(mgi);
            tmgi_set = true;
        }
    }

    //ogs_info(" TMGI [%s], SERVICE ID [%s], PLMN [%s], MCC [%s], MNC [%s]", tmgi_repr, tmgi->mbs_service_id, ogs_plmn_id_to_string(&tmgi->plmn, buf), ogs_plmn_id_mcc_string(&tmgi->plmn), ogs_plmn_id_mnc_string(&tmgi->plmn));
    ogs_free(mcc);
    ogs_free(mnc);

    return tmgi_set;

}

void UserDataIngSession::requiresUserServiceAnnouncement()
{
    const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &mbs_user_data_ing_session = getMBSUserIngSession();

    const std::string &user_service_id = mbs_user_data_ing_session->getMbsUserServId();
    try {
        const std::shared_ptr<UserService> user_service = UserService::find(user_service_id);
        // The bundle write is gated on the broader predicate, not on requiresUserServiceAnnouncement()
        // and an existing carousel channel. TS 26.517 cl.5.2.6 and TS 29.580 cl.6.1.6.2.2 define
        // VIA_MBS_5, VIA_MBS_DISTRIBUTION_SESSION and PASSED_BACK as three independent announcement
        // distribution modes over identical bundle content, and UserServiceAnnBundle and
        // setUserServiceAnnBundler() depend on no carousel channel. Gating on the narrower pair would
        // leave a service declaring VIA_MBS_5 or PASSED_BACK alone with no bundle written at all, and
        // MBS-5 discovery and retrieval for it answering 204 permanently. The carousel-specific
        // bookkeeping below stays on the narrower predicate, which is correctly scoped for it.
        // code-derived: no clause requires this particular gating, only that all three modes are served.
        if(user_service->requiresUserServiceAnnouncementBundle()) {
            setUserServiceAnnBundler();
        }
        if(user_service->requiresUserServiceAnnouncement()) {
            const std::shared_ptr<UserServiceAnnChannel> &ann_channel = App::self().context()->userServiceAnnouncementChannel();
            if(ann_channel) {
                //ann_channel->addUserDataIngSession(user_data_ing_session);
                resetCarouselObject();
                includedInCarouselObjectManifest(false);
                userSerAdNotificationSent(false);
            }
        }
    } catch (std::exception &ex) {
       ogs_error("Unable to find the User Service [%s]", user_service_id.c_str());
    }
}

void UserDataIngSession::configureUserServiceAnnouncementBundler()
{
    const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &mbs_user_data_ing_session = getMBSUserIngSession();

    const std::string &user_service_id = mbs_user_data_ing_session->getMbsUserServId();
    try {
        const std::shared_ptr<UserService> user_service = UserService::find(user_service_id);
        if (!user_service || !checkIfAllMBSDistributionSessionsEstablishedOrActive()) return;

        // Uses the broader announcement predicate for the same reason as
        // requiresUserServiceAnnouncement() above; see its comment.
        if(user_service->requiresUserServiceAnnouncementBundle()) {
            setUserServiceAnnBundler();
        }
        if(user_service->requiresUserServiceAnnouncement()) {
            const std::shared_ptr<UserServiceAnnChannel> &ann_channel = App::self().context()->userServiceAnnouncementChannel();
            if(ann_channel /*&& !user_data_ing_session->getUserServiceAnnBundler()*/) {
                resetCarouselObject();
                includedInCarouselObjectManifest(false);
                userSerAdNotificationSent(false);
            }
        }
    } catch (std::exception &ex) {
       ogs_error("Unable to find the User Service [%s]", user_service_id.c_str());
    }

}

UserDataIngSession &UserDataIngSession::setUserServiceAnnBundler()
{
    if (m_userServiceAnnBundle) {
        m_userServiceAnnBundle->rebuildBundle();
    } else {
        m_userServiceAnnBundle.reset(new UserServiceAnnBundle(weak_from_this().lock()));
    }
    return *this;
}

std::shared_ptr< ObjDistributionOperatingMode > UserDataIngSession::getOperatingMode(const std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getOperatingMode();
    }
    return nullptr;
}

std::shared_ptr< ObjAcquisitionMethod > UserDataIngSession::getAcquisitionMethod(const std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqMethod();
    }
    return nullptr;
}

std::optional<std::string> UserDataIngSession::getObjectIngestUrl(const std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjIngUri();
    }
    return std::nullopt;
}

std::optional<std::string> UserDataIngSession::objectIngestBaseUrl(const std::string &key)
{
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getDistributionSessionInfoData(key);
    if(!context_data || !context_data->distSession) return std::nullopt;
    return getObjectIngestBaseUrl(context_data->distSession);
}


std::optional<std::string> UserDataIngSession::getObjectIngestBaseUrl(const std::shared_ptr<reftools::mbsf::DistSession> &session)
{
    std::optional<std::shared_ptr< reftools::mbsf::ObjDistributionData > > obj_distribution_data =  session->getObjDistributionData();
    if (!obj_distribution_data || !*obj_distribution_data) return std::nullopt;

    return obj_distribution_data.value()->getObjIngestBaseUrl();

}

std::optional<std::string> UserDataIngSession::objectAcquisitionIdPush(const std::string &key)
{
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getDistributionSessionInfoData(key);
    if(!context_data || !context_data->distSession) return std::nullopt;
    return getObjectAcquisitionIdPush(context_data->distSession);
}


std::optional<std::string> UserDataIngSession::getObjectAcquisitionIdPush(const std::shared_ptr<reftools::mbsf::DistSession> &session)
{
    std::optional<std::shared_ptr< reftools::mbsf::ObjDistributionData > > obj_distribution_data =  session->getObjDistributionData();
    if (!obj_distribution_data || !*obj_distribution_data) return std::nullopt;

    return obj_distribution_data.value()->getObjAcquisitionIdPush();

}



std::list<std::optional<std::string>, fiveg_mag_reftools::OgsAllocator<std::optional<std::string>>>
UserDataIngSession::getObjectAcquisitionIds(const std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr<ObjectDistrMethInfo>> obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqIds();
    }
    return {};
}

std::optional<std::string> UserDataIngSession::getObjectDistributionUrl(const std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjDistrUri();
    }
    return std::nullopt;
}

void UserDataIngSession::pendingDeleteResponse(ogs_pool_id_t stream_id)
{
    std::lock_guard<decltype(m_deleteRequestsMutex)::element_type> lock(*m_deleteRequestsMutex);
    m_deleteRequests.push_back(stream_id);
}

std::list<std::shared_ptr<DistributionSessionDesc>> UserDataIngSession::distributionSessionDescs()
{
    std::list<std::shared_ptr<DistributionSessionDesc>> distribution_session_descs = std::list<std::shared_ptr<DistributionSessionDesc>>();
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->distributionSessionInfo) continue;
        std::shared_ptr< DistributionSessionDesc > distribution_session_desc = dist_sess_info.second->distributionSessionInfo->populateDistributionSessionDesc( m_UserDataIngSessionId, dist_sess_info.first);
        distribution_session_descs.push_back(std::move(distribution_session_desc));
    }
    return distribution_session_descs;

}

std::optional<std::list<std::shared_ptr<ServiceScheduleDesc>>>  UserDataIngSession::serviceScheduleDescs()
{
    if(!m_activePeriods) return std::nullopt;
    return m_activePeriods->serviceScheduleDescriptions();
}

void UserDataIngSession::serviceScheduleDescsUpdate(const std::shared_ptr<MBSUserDataIngSession> &mbs_user_data_ing_session)
{
    const std::optional<std::shared_ptr< reftools::mbsf::UserServiceDescription > > &user_service_description = mbs_user_data_ing_session->getMbsUserServiceAnmt();
    if (user_service_description.has_value()) {
        std::shared_ptr< reftools::mbsf::UserServiceDescription > user_service_desc = user_service_description.value();
        //std::optional<std::list<std::optional<std::shared_ptr< ServiceScheduleDescription > >
        reftools::mbsf::UserServiceDescription::ServiceScheduleDescriptionsType  service_schedule_descriptions = user_service_desc->getServiceScheduleDescriptions();
        if (service_schedule_descriptions.has_value()) {
            for(const auto &service_schedule_description : service_schedule_descriptions.value()) {
                 // ServiceScheduleDescriptionsType is std::optional<std::list<std::optional<...>>>: the
                 // outer has_value() above only clears the list itself, not each element's own optional
                 // (same generated-model shape as UserService.cc's fixed bug) -- guard it here too,
                 // matching the per-element check every other consumer of this shape already does
                 // (e.g. ExternalServiceArea.cc's "if (geo_area.has_value())").
                 if (!service_schedule_description.has_value()) continue;
                 const std::string &id = service_schedule_description.value()->getId();
                 {
                     std::lock_guard<decltype(m_serviceScheduleDescMutex)::element_type> lock(*m_serviceScheduleDescMutex);
                     auto it =  m_serviceScheduleDescs.find(id);
                     if (it !=  m_serviceScheduleDescs.end()) {
                         std::shared_ptr< ServiceScheduleDesc > schedule = it->second;
                         std::shared_ptr< reftools::mbsf::ServiceScheduleDescription > new_service_schedule_description = service_schedule_description.value();
                         std::shared_ptr< reftools::mbsf::ServiceScheduleDescription > current_service_schedule_description = schedule->serviceScheduleDescription();
                         if (*new_service_schedule_description == *current_service_schedule_description) {
                             continue;
                         } else {
                            schedule->changeServiceScheduleDescription(new_service_schedule_description);

                         }

                      }
                 }
            }
        }
    }
}

std::shared_ptr<UserServiceDesc> UserDataIngSession::userServiceDesc()
{
   std::shared_ptr<UserServiceDesc> user_service_desc = nullptr;
   const std::string &mbs_user_service_id = m_MBSUserDataIngSession->getMbsUserServId();
   if (mbs_user_service_id.empty()) return nullptr;

   try {
       const std::shared_ptr<UserService> &mbs_user_service = UserService::find(mbs_user_service_id);
       user_service_desc.reset(new UserServiceDesc(mbs_user_service->serviceIds(), mbs_user_service->serviceClass(),
                              mbs_user_service->UserServiceDescriptionNames(), mbs_user_service->UserServiceDescriptionDescs(),
                              distributionSessionDescs(), serviceScheduleDescs()) );
   } catch (std::out_of_range &ex) {
       ogs_error("Unable to find the parent MBS User Service");
       return nullptr;
   }
   return user_service_desc;
}

const std::shared_ptr<UserService> &UserDataIngSession::mbsUserService()
{
    static const std::shared_ptr<UserService> null_retval(nullptr);
    const std::string &mbs_user_service_id = m_MBSUserDataIngSession->getMbsUserServId();
    if (mbs_user_service_id.empty()) return null_retval;

    try {
        const std::shared_ptr<UserService> &mbs_user_service = UserService::find(mbs_user_service_id);
        return mbs_user_service;
    } catch (std::out_of_range &ex) {
        ogs_error("Unable to find the parent MBS User Service");
        return null_retval;
    }
    return null_retval;

}

bool UserDataIngSession::isMBSSessionCreated(const std::string &key)
{
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data) return context_data->MBSSessionStatus == MBSSessionState::CREATED;
    return false;
}

bool UserDataIngSession::hasMbstfResponded(const std::string &key)
{
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data) return context_data->receivedMBSTFResponse;
    return false;
}

const DistSessionState &UserDataIngSession::lastReportedState(const std::string &key) const
{
    static const DistSessionState no_val_state;

    std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data) return context_data->last_reported_state;
    return no_val_state;
}

bool UserDataIngSession::inDesiredState(const std::string &key)
{
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data) return !context_data->stateUpdate && !context_data->needsUpdate;
    return true;
}

const DistSessionState &UserDataIngSession::stateOfDistSession(const std::string &key)
{
    static const DistSessionState session_state_no_val;
    std::shared_ptr<UserDataIngSession::ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data && context_data->distSession) {
        return *context_data->distSession->getDistSessionState();
    }
    return session_state_no_val;
}

bool UserDataIngSession::distributionSessionInfoHasMbstfandMbsSession(const std::string &key)
{
    std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data && !context_data->mbstfNFInstanceId.empty() && context_data->MBSSession) {
        return true;
    }
    return false;
}

bool UserDataIngSession::userDataIngSessionForServiceAnnChannel(const std::shared_ptr<UserDataIngDistSessId> &ids)
{
    if (ids->second == "USER SERVICE ANNOUNCEMENT CHANNEL") return true;
    return false;
}

bool UserDataIngSession::isUserServiceAnnouncementChannel(const std::string &distribution_session_info_key)
{
    if (distribution_session_info_key == "USER SERVICE ANNOUNCEMENT CHANNEL") return true;
    return false;
}

void UserDataIngSession::populateCarouselObject(const std::shared_ptr<Open5GSSBINFInstance> &nf_instance)
{
    // Find first address available and use that for the carousel object

    int number_of_ipv6 = nf_instance->numberOfIPv6();
    if (number_of_ipv6) {
        ogs_sockaddr_t **addrs_v6 = nf_instance->Ipv6();
        for (int i = 0; i < number_of_ipv6; i++) {
            if (addrs_v6[i]) {
                ogs_sockaddr_t *addr_v6 = addrs_v6[i];
                reftools::common::httpxpp::SockAddr remote_mbstf_sock_addr(addr_v6->sa);
                try {
                    const reftools::common::httpxpp::SockAddr &svr_sock_addr =  App::self().context()->findUserServAnnServerAddrForRemote(remote_mbstf_sock_addr);
                    populateObjectCarousel(std::format("{}", svr_sock_addr));
                    return;
                } catch (std::out_of_range &ex) {
                    // go onto next address if we couldn't find a server port that will serve the NF address
                }
            }
        }
    }

    int number_of_ipv4 = nf_instance->numberOfIPv4();
    if (number_of_ipv4) {
        ogs_sockaddr_t **addrs = nf_instance->Ipv4();
        for (int i = 0; i < number_of_ipv4; i++) {
            if (addrs[i]) {
                ogs_sockaddr_t *addr = addrs[i];
                reftools::common::httpxpp::SockAddr remote_mbstf_sock_addr(addr->sa);
                try {
                    const reftools::common::httpxpp::SockAddr &svr_sock_addr = App::self().context()->findUserServAnnServerAddrForRemote(remote_mbstf_sock_addr);
                    populateObjectCarousel(std::format("{}", svr_sock_addr));
                    return;
                } catch (std::out_of_range &ex) {
                    // go onto next address if we couldn't find a server port that will serve the NF address
                }
            }
        }
    }
}

void UserDataIngSession::populateObjectCarousel(const std::string &user_serv_ann_server_addr)
{
    if (user_serv_ann_server_addr.empty()) return;

    std::string object_locator = std::format("http://{}/x-5gmag-service-announcements/v1/user-data-ingest-session/{}", user_serv_ann_server_addr, m_UserDataIngSessionId);
    std::shared_ptr<CarouselObject> object(new CarouselObject(object_locator, App::self().context()->repetitionInterval(),
                                                               App::self().context()->keepUpdated()));
    setCarouselObject(object);
}

void UserDataIngSession::userServiceAnnBundled()
{
    // userServiceAnnBundleAvailable(true) is set here, before the carousel-specific early return
    // below. This runs immediately after UserServiceAnnBundle::worker() has written announcement.json
    // and every dependent SDP file (writeAnnouncement() and writeServiceDescriptionProtocolDoc() have
    // both returned by now), so the bundle's content is on disk whether or not an MBS-4-MC carousel
    // channel exists. The rest of this function is carousel-specific and correctly returns early
    // without one; setting the flag from inside that path instead would leave a VIA_MBS_5-only or
    // PASSED_BACK-only service's written bundle never reported available, and
    // UserServiceDiscoveryHandler's isUserServiceAnnBundleAvailable() check false, so MBS-5 discovery
    // and retrieval would answer 204 with the files already present. code-derived, no spec claim.
    userServiceAnnBundleAvailable(true);

    std::shared_ptr<Open5GSSBINFInstance> nf_instance = nullptr;

    const std::shared_ptr<UserServiceAnnChannel> &ann_channel = App::self().context()->userServiceAnnouncementChannel();
    if(!ann_channel) {
        return;
    }

    const std::shared_ptr<UserDataIngSession> &ann_channel_user_data_ing_session = ann_channel->annChannelUserDataIngSession();
    if(!ann_channel_user_data_ing_session) return;
    std::shared_ptr< UserDataIngSession::ContextData > context_data = ann_channel_user_data_ing_session->getDistributionSessionInfoData(ann_channel->key());
    if(!context_data || context_data->mbstfNFInstanceId.empty())
    {
        return;
    }

    try {
        nf_instance.reset(new Open5GSSBINFInstance(context_data->mbstfNFInstanceId, false));
    } catch (const std::runtime_error &ex) {
        // Open5GSSBINFInstance's by-id constructor throws, rather than leaving the pointer null, when
        // MBSTF's NF instance is not yet in MBSF's local SBI NF instance cache, which is a normal timing
        // gap rather than an error. The "if(!nf_instance)" fallback below is written for exactly this
        // case, so the exception is caught and takes that same path; uncaught it would end the process
        // through std::terminate().
        nf_instance = nullptr;
    }
    if(!nf_instance)
    {
        ann_channel_user_data_ing_session->nmbstfDiscoverOnly(context_data);
        return;
    }

    try {
        populateCarouselObject(nf_instance);
    } catch (std::exception &ex) {
        ogs_error("Failed to populate Carousel Object Manifest:[%s]", ex.what());
        return;
    }

    std::lock_guard<decltype(m_carouselObjectMutex)::element_type> lock(*m_carouselObjectMutex);
    if (m_carouselObject) {
        //m_carouselObjectManifest.reset(new ObjManifest(objects, object_locators));
        userServiceAnnBundleAvailable(true);
        ann_channel->addUserDataIngSession(weak_from_this().lock());
        pushNotificationsEvent();
    } else {
        ogs_debug("No Objects for carousel object manifest");
    }
}

void UserDataIngSession::setCarouselObject(const std::shared_ptr<CarouselObject> &carousel_object)
{
    std::lock_guard<decltype(m_carouselObjectMutex)::element_type> lock(*m_carouselObjectMutex);
    m_carouselObject = carousel_object;
}

std::shared_ptr<CarouselObject> UserDataIngSession::getCarouselObject() const
{
    std::lock_guard<decltype(m_carouselObjectMutex)::element_type> lock(*m_carouselObjectMutex);
    return m_carouselObject;
}

void UserDataIngSession::resetCarouselObject()
{
    std::lock_guard<decltype(m_carouselObjectMutex)::element_type> lock(*m_carouselObjectMutex);
    m_carouselObject.reset();
}


void UserDataIngSession::forEachObjectLocator(std::function<void(const std::string &)> fn) const
{
    std::lock_guard<decltype(m_carouselObjectMutex)::element_type> lock(*m_carouselObjectMutex);
    if (m_carouselObject) fn(m_carouselObject->object()->getLocator());
}

void UserDataIngSession::userSerAdNotificationSent(bool notification_sent) const
{
    m_userSerAdNotificationSent = notification_sent;
}

const DistSessionState &UserDataIngSession::getDistributionSessionInfoState(const std::string &key) const
{
    static const DistSessionState state_no_val;
    std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);
    if (context_data) {
        const auto &state = context_data->info->getMbsDistSessState();
        if (state) return *state.value();
    }
    return state_no_val;
}

void UserDataIngSession::setDistSessionState(const std::shared_ptr<DistSessionState> &state)
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (auto &[dist_sess_id, context_data] : m_distributionSessionInfos) {
        const auto &dist_sess_state = context_data->info->getMbsDistSessState();
        if (dist_sess_state.has_value()) {
            if (dist_sess_state.value()->getValue() == state->getValue()) {
                context_data->needsUpdate = false;
                context_data->stateUpdate = false;
                continue;
            }
        }
        std::shared_ptr<UserDataIngDistSessId> ids_ptr(new UserDataIngDistSessId{m_UserDataIngSessionId, dist_sess_id});
        context_data->stateUpdate = true;
        context_data->info->setMbsDistSessState(state);
        sendMbsmfActivityStatus(ids_ptr);
        sendNotificationsEvent(ids_ptr);

        if (context_data->needsUpdate || (dist_sess_state && *dist_sess_state.value() != context_data->last_reported_state)) {
            SessionIdContainer session_id{context_data->mbstfDistSessionId, ids_ptr};
            nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSessionPatch, nullptr, &session_id);
        }
    }
}

/**** Local functions ****/

static void process_mbs_distribution_session_info(const std::shared_ptr<UserDataIngSession::ContextData> &context_data, const std::shared_ptr<DistSession> &dist_session)
{
    if (!context_data || !context_data->info || !dist_session) return;
    auto &obj_distribution_method_info = context_data->info->getObjDistrInfo();
    if (obj_distribution_method_info.has_value()) {
        auto &obj_dist_method_info = obj_distribution_method_info.value();
        if (obj_dist_method_info->getOperatingMode()->getString() == "PUSH") {
            auto &obj_distribution_data = dist_session->getObjDistributionData();
            if (obj_distribution_data.has_value()) {
                auto &obj_dist_data = obj_distribution_data.value();
                auto &obj_acquisition_method = obj_dist_data->getObjAcquisitionMethod();
                if (obj_acquisition_method->getString() == "PUSH") {
                    obj_dist_method_info->setObjIngUri(obj_dist_data->getObjIngestBaseUrl());
                }
            }
        }
    }
    auto &pkt_distribution_method_info = context_data->info->getPckDistrInfo();
    if (pkt_distribution_method_info) {
        auto &pkt_ing_endpoint_addr = pkt_distribution_method_info.value()->getIngEndpointAddrs();
        auto &pkt_distribution_data = dist_session->getPktDistributionData();
        if (ogs_likely(pkt_distribution_data)) {
            auto &pkt_mbstf_ingest_addr = pkt_distribution_data.value()->getMbStfIngestAddr();
            // merge the MBSTF values from mbStfIngestAddr into the existing ingEndpointAddrs (both MbStfIngestAddr types).
            pkt_ing_endpoint_addr->setMbStfIngressTunAddr(pkt_mbstf_ingest_addr->getMbStfIngressTunAddr());
            pkt_ing_endpoint_addr->setMbStfListenAddr(pkt_mbstf_ingest_addr->getMbStfListenAddr());
        }
    }
}

static bool resolve_src_dest_addr(const std::string &src_addr, const std::string &dest_addr, struct addrinfo **src_addrinfo, struct addrinfo **dest_addrinfo)
{
        ogs_debug("resolving SSM");
        int result;

        result = getaddrinfo(src_addr.c_str(), NULL, NULL, src_addrinfo);
        if (result) {
            ogs_error("Unable to resolve SSM source address '%s': %s", src_addr.c_str(), gai_strerror(result));
            if (src_addrinfo && *src_addrinfo) {
                freeaddrinfo(*src_addrinfo);
                *src_addrinfo = NULL;
            }
            return false;
        }

        result = getaddrinfo(dest_addr.c_str(), NULL, NULL, dest_addrinfo);
        if (result) {
            ogs_error("Unable to resolve SSM multicast destination address '%s': %s", dest_addr.c_str(), gai_strerror(result));
            if (src_addrinfo && *src_addrinfo) {
                freeaddrinfo(*src_addrinfo);
                *src_addrinfo = NULL;
            }
            if (dest_addrinfo && *dest_addrinfo) {
                freeaddrinfo(*dest_addrinfo);
                *dest_addrinfo = NULL;
            }
            return false;
        }
        return true;

}

static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr)
{
    while (src_addrinfo && src_addrinfo->ai_family != family) src_addrinfo = src_addrinfo->ai_next;
    if (!src_addrinfo) return false;

    while (dest_addrinfo && dest_addrinfo->ai_family != family) dest_addrinfo = dest_addrinfo->ai_next;
    if (!dest_addrinfo) return false;

    if (family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in*)src_addrinfo->ai_addr;
        *src_addr = &addr->sin_addr;
        addr = (struct sockaddr_in*)dest_addrinfo->ai_addr;
        *dest_addr =  &addr->sin_addr;
    } else if (family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6*)src_addrinfo->ai_addr;
        *src_addr = &addr->sin6_addr;
        addr = (struct sockaddr_in6*)dest_addrinfo->ai_addr;
        *dest_addr = &addr->sin6_addr;
    } else {
        *src_addr = NULL;
        *dest_addr = NULL;
    }
    return true;
}

static std::string print_mbs_session_error(const std::shared_ptr<UserDataIngSession::ContextData> &context_data) {
    // ssm and MBSSession are both checked rather than dereferenced. ssm is null whenever the
    // Distribution Session carries no AF-supplied SSM, the normal case in which this MBSF nominates
    // its own broadcastDistribution address instead (see the .ssm = nullptr construction sites
    // above), and MBSSession is null whenever the MB-SMF Create that would have populated it did not
    // succeed. Both hold on the failure path that reaches this function,
    // setMBSSessionFailureFlag() -> populateAndSendError(), so either is treated as absent, the same
    // way the loop below treats an empty optional.
    auto ssm_val = context_data->ssm;
    std::optional<std::string> src_ipv4_addr, dest_ipv4_addr, src_ipv6_addr, dest_ipv6_addr;
    if (ssm_val) {
        auto src_ip_addr  = ssm_val->getSourceIpAddr();
        auto dest_ip_addr = ssm_val->getDestIpAddr();

        src_ipv4_addr  = src_ip_addr->getIpv4Addr();
        dest_ipv4_addr = dest_ip_addr->getIpv4Addr();

        const std::optional<std::shared_ptr<Ipv6Addr>> &src_ipv6_addr_obj  = src_ip_addr->getIpv6Addr();
        const std::optional<std::shared_ptr<Ipv6Addr>> &dest_ipv6_addr_obj = dest_ip_addr->getIpv6Addr();
        src_ipv6_addr  = src_ipv6_addr_obj?std::make_optional<std::string>(*src_ipv6_addr_obj.value()):std::nullopt;
        dest_ipv6_addr = dest_ipv6_addr_obj?std::make_optional<std::string>(*dest_ipv6_addr_obj.value()):std::nullopt;
    }

    std::optional<std::string> tmgi_opt;
    if (context_data->MBSSession) {
        const char* tmgi_cstr = context_data->MBSSession->tmgi();
        ogs_debug("TMGI Error: %s", tmgi_cstr);
        if (tmgi_cstr && *tmgi_cstr) {
            tmgi_opt = tmgi_cstr;
        }
    }

    std::vector<std::pair<const char*, const std::optional<std::string>*>> fields = {
        { "tmgi",            &tmgi_opt      },
        { "sourceIpAddr",    &src_ipv4_addr },
        { "destIpAddr",      &dest_ipv4_addr},
        { "sourceIpv6Addr",  &src_ipv6_addr },
        { "destIpv6Addr",    &dest_ipv6_addr}
    };

    std::ostringstream oss;
    oss << "MBS Session ID [";

    bool first = true;
    for (auto const& [label, optPtr] : fields) {
        if (!*optPtr) continue;               // skip empty
        if (!first)   oss << ", ";
        oss << label << ": " << **optPtr;
        first = false;
    }

    oss << "] could not be created\n";
    return oss.str();

}

static void handle_failed_mbstf_nf_instance_discover(ogs_sbi_xact_t *xact)
{
    ogs_sbi_stream_t *ogs_stream = reinterpret_cast<ogs_sbi_stream_t*>(ogs_sbi_stream_find_by_id(xact->assoc_stream_id));
    if (!ogs_stream) return;
    Open5GSSBIStream stream(xact->assoc_stream_id);

    ogs_assert(true == Open5GSSBIServer::sendError(stream, OGS_SBI_HTTP_STATUS_INTERNAL_SERVER_ERROR, std::nullopt,
                                 "Unable to Discover MBSTF", "MBSTF discovery failed" , "No MBSTF found in the network"));

}

static int64_t duration_timer(const std::chrono::system_clock::time_point &tp) {
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(tp - now).count();
    if (diff <= 0) return 0;
    return static_cast<int64_t>(diff);
}

static bool request_too_large(Open5GSSBIRequest &request, Open5GSSBIStream &stream, int path_segments,
                              Open5GSSBIMessage &message, const NfServer::AppMetadata &app_meta,
                              const std::optional<NfServer::InterfaceMetadata> &api)
{
    const auto &max_size = App::self().context()->maxRequestBodySize;
    if (!max_size || request.contentLength() <= *max_size) return false;

    std::ostringstream err;
    err << "Request body of " << request.contentLength() << " bytes exceeds the configured maximum of "
        << *max_size << " bytes";
    ogs_error("%s", err.str().c_str());
    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_PAYLOAD_TOO_LARGE, path_segments, message,
                                            app_meta, api, "Payload Too Large", err.str()));
    return true;
}

/* TS 29.500 V18.10.0 cl.6.6.2 (feature negotiation): the server determines the negotiated
 * features by comparing the client's requested bitmask against its own supported set, and
 * returns the intersection. TS 29.580 V18.8.0 cl.6.2.8, Table 6.2.8-1 (Nmbsf_MBSUserDataIngestSession
 * API) defines exactly four features: 1 5MBS2, 2 MBSEventsExt, 3 MBSErrorHandling, 4 MBSPatchEnh.
 * Of these, only MBSEventsExt (feature 2) is genuinely implemented in this repository -- this
  * mask reflects what's actually there, not an aspiration. TS 29.571's own
 * SupportedFeatures encoding (its own copy of TS 29.500 table 5.2.2-3): each hex character
 * represents 4 features, and the last character in the string represents features 1 to 4 -- since
 * this API defines no feature above 4, only that last character is ever relevant here. */
static const unsigned MBSF_UD_INGEST_SUPPORTED_FEATURES = 0x2; /* bit 1 = feature 2 = MBSEventsExt */

static std::optional<std::string> negotiate_supp_feat(const std::optional<std::string> &requested)
{
    /* TS 29.580 V18.8.0 cl.6.2.6.2.2 (suppFeat): "This attribute shall be present in an HTTP
     * POST/PUT request and response, if feature negotiation needs to take place." -- absent
     * request means no negotiation takes place, not "negotiate to nothing". */
    if (!requested) return std::nullopt;

    unsigned requested_mask = 0;
    if (!requested->empty()) {
        char last = requested->back();
        if (last >= '0' && last <= '9') requested_mask = (unsigned)(last - '0');
        else if (last >= 'a' && last <= 'f') requested_mask = (unsigned)(last - 'a' + 10);
        else if (last >= 'A' && last <= 'F') requested_mask = (unsigned)(last - 'A' + 10);
    }

    unsigned negotiated = requested_mask & MBSF_UD_INGEST_SUPPORTED_FEATURES;
    static const char hex_digits[] = "0123456789abcdef";
    return std::string(1, hex_digits[negotiated & 0xf]);
}

static bool validate_state_setting_options(const std::shared_ptr<UserDataIngSession> &user_data_ing_session,
                                           Open5GSSBIStream &stream, Open5GSSBIMessage &message,
                                           const NfServer::AppMetadata &app_meta,
                                           const std::optional<NfServer::InterfaceMetadata> &api)
{
    std::shared_ptr<MBSUserDataIngSession> mbs_user_data_ing_session = user_data_ing_session->getMBSUserIngSession();
    std::map<std::string,std::string> invalid_params;
    if (mbs_user_data_ing_session->getActPeriods() && mbs_user_data_ing_session->getActPeriodsRepRule()) {
        invalid_params["actPeriods"] = "actPeriods cannot be present if any mbsDistSessState or actPeriodRepRule are present";
        invalid_params["actPeriodRepRule"] = "actPeriodRepRule cannot be present if any mbsDistSessState or actPeriods are present";
    }
    std::shared_ptr<Context> context = App::self().context();
    for (auto &[dist_sess_id, dist_sess_info] : mbs_user_data_ing_session->getMbsDisSessInfos()) {
        if (dist_sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = dist_sess_info.value();
            if (info) {
                std::optional<std::shared_ptr< DistSessionState > > dist_session_state = info->getMbsDistSessState();
                if (dist_session_state.has_value()){
                    std::shared_ptr< DistSessionState > dist_sess_state = dist_session_state.value();
                    if (dist_sess_state->getString() == "DEACTIVATING") {
                        invalid_params[std::format("mbsDisSessInfos.{}.mbsDistSessState", dist_sess_id)] = "mbsDistSessState cannot be DEACTIVATING";
                    }
                }
                if (dist_session_state.has_value() && mbs_user_data_ing_session->getActPeriods()) {
                   invalid_params["actPeriods"] = "actPeriods cannot be present if any mbsDistSessState or actPeriodRepRule are present";
                   invalid_params[std::format("mbsDisSessInfos.{}.mbsDistSessState", dist_sess_id)] = "mbsDistSessState cannot be present if actPeriods or actPeriodRepRule are present";
                }

                const auto &mbs_session_id = info->getMbsSessionId();
                const auto &mbs_service_area = info->getTgtServAreas();
                const auto &ext_mbs_service_area = info->getExtTgtServAreas();
                if (mbs_session_id) {
                    UniqueMbsSessionId unique_mbs_session_id(!!mbs_session_id.value()->getSsm(), mbs_session_id.value(),
                                    mbs_service_area?mbs_service_area.value():std::shared_ptr<MbsServiceArea>(),
                                    ext_mbs_service_area?ext_mbs_service_area.value():std::shared_ptr<ExternalMbsServiceArea>());
                    if (context->haveMbsSessionId(unique_mbs_session_id)) {
                        invalid_params[std::format("mbsDisSessInfos.{}.mbsSessionId", dist_sess_id)] = "mbsSessionId already used in another UserDataIngSession";
                    }
                }

                // tgtServAreas and extTgtServAreas are mutually exclusive (TS 29.580).
                if (mbs_service_area.has_value() && ext_mbs_service_area.has_value()) {
                    invalid_params[std::format("mbsDisSessInfos.{}.tgtServAreas", dist_sess_id)] =
                        "tgtServAreas and extTgtServAreas are mutually exclusive";
                }

                // Per TS 29.580, nrRedCapUeInfo and mbsFSAId are broadcast-only and restrictedFlag is
                // multicast-only, so each is checked against the parent MBS User Service's servType.
                const std::shared_ptr<UserService> &parent_user_service = user_data_ing_session->mbsUserService();
                const std::string serv_type = parent_user_service ? parent_user_service->getMBSUserServiceType() : std::string();
                if (serv_type != "BROADCAST") {
                    if (info->getNrRedCapUeInfo().has_value()) {
                        invalid_params[std::format("mbsDisSessInfos.{}.nrRedCapUeInfo", dist_sess_id)] =
                            "nrRedCapUeInfo is only applicable to a BROADCAST MBS User Service";
                    }
                    if (info->getMbsFSAId().has_value()) {
                        invalid_params[std::format("mbsDisSessInfos.{}.mbsFSAId", dist_sess_id)] =
                            "mbsFSAId is only applicable to a BROADCAST MBS User Service";
                    }
                }
                if (serv_type != "MULTICAST" && info->getRestrictedFlag().has_value()) {
                    invalid_params[std::format("mbsDisSessInfos.{}.restrictedFlag", dist_sess_id)] =
                        "restrictedFlag is only applicable to a MULTICAST MBS User Service";
                }

                // TS 29.580 V18.8.0 table 5.6.2.8-1, pckIngMethod row: "When the "operatingMode"
                // attribute is set to "PACKET_FORWARD_ONLY", only the value "UNICAST" is applicable
                // for this attribute."
                const auto &pkt_distr_info = info->getPckDistrInfo();
                if (pkt_distr_info) {
                    const auto &oper_mode = pkt_distr_info.value()->getOperatingMode();
                    const auto &ing_method = pkt_distr_info.value()->getPckIngMethod();
                    if (oper_mode && ing_method &&
                        oper_mode->getValue() == PktDistributionOperatingMode::VAL_PACKET_FORWARD_ONLY &&
                        ing_method->getValue() != PktIngestMethod::VAL_UNICAST) {
                        invalid_params[std::format("mbsDisSessInfos.{}.pckDistrInfo.pckIngMethod", dist_sess_id)] =
                            "only UNICAST is applicable when operatingMode is PACKET_FORWARD_ONLY";
                    }
                }
            }
        }
    }
    if (!invalid_params.empty()) {
        ogs_assert(true == NfServer::sendError(stream, ProblemCause::OPTIONAL_IE_INCORRECT, 0, message,
                                                            app_meta, api, std::nullopt, std::nullopt, std::nullopt, invalid_params));

        return false;
    } else {
        return true;
    }
}

static std::shared_ptr<MBSMFMBSSession> populate_mb_smf_mbs_session(
                const std::shared_ptr<UserDataIngSession::ContextData> &context_data,
                const std::shared_ptr<MBSMFMBSSession> &mb_smf_mbs_session) {

    auto &dist_session_state = context_data->info->getMbsDistSessState();
    if (dist_session_state.has_value()) {

        std::shared_ptr< DistSessionState > dist_sess_state = dist_session_state.value();

        if (*dist_sess_state == DistSessionState::VAL_ACTIVE ||
                *dist_sess_state == DistSessionState::VAL_ESTABLISHED) {
            mb_smf_mbs_session->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_ACTIVE);
        } else {

            mb_smf_mbs_session->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_INACTIVE);
        }
    }

    const std::optional<std::shared_ptr< MbsServiceInfo > > &mbs_service_info = context_data->info->getMbsServInfo();

    if (mbs_service_info.has_value()) {
        mb_smf_mbs_session->setServiceInfo(mbs_service_info.value());
    }
    const std::optional<std::string > &mbs_fsa_id = context_data->info->getMbsFSAId();
    if (mbs_fsa_id.has_value()) {
        mb_smf_mbs_session->setFsaId(mbs_fsa_id.value());

    }

    std::optional<bool > location_dependent = context_data->info->getLocationDependent();

    if (location_dependent.has_value() ) {
        mb_smf_mbs_session->setLocationDependent(location_dependent.value());
    }

    const std::optional<std::shared_ptr< MbsServiceArea > > &mbs_service_area = context_data->info->getTgtServAreas();
    if (mbs_service_area.has_value()) {

        mb_smf_mbs_session->setServiceArea(mbs_service_area.value());
    }

    const std::optional<std::shared_ptr< ExternalMbsServiceArea > > &ext_mbs_service_area = context_data->info->getExtTgtServAreas();
    if (ext_mbs_service_area.has_value()) {

        mb_smf_mbs_session->setExternalServiceArea(ext_mbs_service_area.value());
    }

    const std::optional<std::shared_ptr< AssociatedSessionId > > &associated_session_id = context_data->info->getAssociatedSessionId();
    if (associated_session_id.has_value()) {
        mb_smf_mbs_session->setAssociatedSessionId(associated_session_id.value());

    }

    std::optional<bool > restricted_flag = context_data->info->getRestrictedFlag();
    if (restricted_flag.has_value()) {
        mb_smf_mbs_session->setAnyUeInd(restricted_flag.value());
    }

    context_data->MBSSession = mb_smf_mbs_session;
    mb_smf_mbs_session->pushChanges();

    return mb_smf_mbs_session;
}


static void send_invalid_user_data_ing_session_err(const std::out_of_range &e, Open5GSSBIStream &stream,
                                                   size_t number_of_components, const Open5GSSBIMessage &message,
                                                   const NfServer::AppMetadata &app_meta,
                                                   const std::optional<NfServer::InterfaceMetadata> &api,
                                                   const std::string &user_data_ing_session_id)
{

    std::ostringstream err;
    err << "User Data Ingest Session [" << user_data_ing_session_id << "] does not exist.";
    ogs_error("%s", err.str().c_str());

    static const std::string param("{sessionId}");
    std::ostringstream reason;
    reason << "Invalid User Data Ingest Session identifier [" << user_data_ing_session_id << "]";
    std::map<std::string, std::string> invalid_params(NfServer::makeInvalidParams(param, reason.str()));
    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, number_of_components, message,
                                                                    app_meta, api, "User Data Ingest Session not found",
                                                                    err.str(), std::nullopt, invalid_params));


}

static uint64_t get_next_tsi()
{
    uint64_t ret = g_next_tsi++;
    if (g_next_tsi < 2) g_next_tsi = 2;
    return ret;
}

static void send_model_error(const ModelException &err, Open5GSSBIStream &stream, int path_segments, Open5GSSBIMessage &message,
                             const NfServer::AppMetadata &app_meta, const std::optional<NfServer::InterfaceMetadata> &api,
                             const std::string &no_cause_reason, const std::string &log_prefix)
{
    std::ostringstream error_oss;
    std::ostringstream oss;
    std::optional<std::map<std::string,std::string> > invalid_params = std::nullopt;

    if (!err.parameter.empty()) {
        invalid_params = std::map<std::string,std::string>{ {err.parameter, err.what()} };
        error_oss << err.parameter << ": ";
    }
    error_oss << err.what();
    const std::string &error = error_oss.str();

    if (err.cause) {
        auto cause = err.cause.value();
        oss << cause.reason() << ": " << error;
        ogs_assert(true == NfServer::sendError(stream, cause, path_segments, message, app_meta, api, cause.reason(), error, std::nullopt,
                                               invalid_params));
    } else {
        oss << no_cause_reason << ": " << error;
        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, path_segments, message, app_meta, api, no_cause_reason,
                                               error));
    }
    ogs_error("%s: %s", log_prefix.c_str(), oss.str().c_str());
}

static void log_missing_ing_session(const std::string &id) {
    std::ostringstream err;
    err << "MBS User Data Ingest Session [" << id << "] does not exist.";
    ogs_error("%s", err.str().c_str());
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

