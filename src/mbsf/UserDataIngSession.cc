/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS User Data Ingest Session class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// standard template library includes
#include <chrono>
#include <memory>
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
#include "DistributionSessionInfo.hh"
#include "hash.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Nmb2Build.hh"
#include "Open5GSEvent.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBIObject.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIClient.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSNetworkFunction.hh"
#include "UserService.hh"
#include "UniqueMBSSessionId.hh"
#include "UserDataIngSessionNotificationEvent.hh"
#include "openapi/model/DistSession.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/Tmgi.h"
#include "openapi/model/TunnelAddress.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/PlmnId.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/ProblemCause.hh"

#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/ObjectDistrMethInfo.h"


#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"
#include "mb-smf-service-consumer.h"
#include "TimerFunc.hh"

// Header include for this class
#include "UserDataIngSession.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using fiveg_mag_reftools::ProblemCause;
using reftools::mbsf::AssociatedSessionId;
using reftools::mbsf::DistSession;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::IpAddr;
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
using reftools::mbsf::Ssm;
using reftools::mbsf::Tmgi;
using reftools::mbsf::TunnelAddress;

MBSF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbsf_userdataingsession_api_metadata(
    NMBSF_MBS_UD_INGEST_API_NAME,
    NMBSF_MBS_UD_INGEST_API_VERSION
);

using ActPeriodsType = MBSUserDataIngSession::ActPeriodsType;
using ActPeriodsRepRuleType = MBSUserDataIngSession::ActPeriodsRepRuleType;

static bool resolve_src_dest_addr(const std::string &src_ipv4_addr, const std::string &dest_ipv4_addr, struct addrinfo **ai_src, struct addrinfo **ai_dest);
static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr);
static void process_mbs_distribution_session_info(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::shared_ptr< DistSession > dist_session);
static std::string print_mbs_session_error(std::shared_ptr< UserDataIngSession::ContextData > context_data);
static void handle_failed_mbstf_nf_instance_discover(ogs_sbi_xact_t *xact);
static bool validate_state_setting_options(std::shared_ptr<UserDataIngSession> user_data_ing_session, Open5GSSBIStream &stream, Open5GSSBIMessage &message, const NfServer::AppMetadata &app_meta, std::optional<NfServer::InterfaceMetadata> api);
static void send_invalid_user_data_ing_session_err(const std::out_of_range &e, Open5GSSBIStream &stream, size_t number_of_components,
                const Open5GSSBIMessage &message, const NfServer::AppMetadata &app_meta, std::optional<NfServer::InterfaceMetadata> api, const std::string &user_data_ing_session_id);
/*
static std::shared_ptr<MBSDistributionSessionInfo> change_mbs_dist_session_infos(std::shared_ptr< UserDataIngSession::ContextData > context_data,
    std::shared_ptr<MBSDistributionSessionInfo> current_mbs_dist_session_infos,
    std::shared_ptr<MBSDistributionSessionInfo> new_mbs_dist_session_infos);
*/

static std::shared_ptr<MBSMFMBSSession> populate_mb_smf_mbs_session(std::shared_ptr< UserDataIngSession::ContextData > context_data,
                std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session);

static std::int64_t duration_timer(const std::chrono::system_clock::time_point &tp);

std::recursive_mutex UserDataIngSession::s_registry_mutex;
std::map<ogs_sbi_xact_t *, std::shared_ptr< UserDataIngSession::UserDataIngDistSessId >> UserDataIngSession::s_xactRegistry;
std::map<std::string, std::shared_ptr< UserDataIngSession::UserDataIngDistSessId >> UserDataIngSession::s_distSessionIdRegistry;

UserDataIngSession::UserDataIngSession(CJson &json, bool as_request)
    :m_MBSUserDataIngSession(new MBSUserDataIngSession(json, as_request))
    ,m_sbiObject(new Open5GSSBIObject)
    ,m_generated()
    ,m_lastUsed()
    ,m_hash()
    ,m_UserDataIngSessionId()
    ,m_alwaysActive(nullptr)
    ,m_activePeriods(nullptr)
    ,m_activePeriodsRepRule(nullptr)
    ,m_activePeriodsTimer(nullptr)
    ,m_distSessionState()
    ,m_currentDistSessionState()
    ,m_desiredDistSessionState()
    ,m_startTimer(true)
    ,m_distributionSessionInfos()
    ,m_distSessInfosMutex(new decltype(m_distSessInfosMutex)::element_type)
    ,m_deleteRequestsMutex(new decltype(m_deleteRequestsMutex)::element_type)
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

    m_distSessionState = DistSessionState::VAL_INACTIVE;
    m_currentDistSessionState = DistSessionState::NO_VAL;
    m_desiredDistSessionState = DistSessionState::NO_VAL;
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

int UserDataIngSession::numberOfDistributionSessions()
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    int count = 0;
    std::map<std::string, std::shared_ptr< UserDataIngDistSessId >>::size_type size = s_distSessionIdRegistry.size();
    if (size <= static_cast<std::map<std::string, std::shared_ptr< UserDataIngDistSessId >>::size_type>(std::numeric_limits<int>::max())) {
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

            std::shared_ptr<Open5GSSBIRequest> request_ctx(new Open5GSSBIRequest(message));

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
                            if (ex.cause) {
                                  ogs_assert(true == NfServer::sendError(stream, ex.cause.value(), 3, message, app_meta,
                                                    api, /*ex.cause.value().reason() */ "Mandatory information element missing", ex.what(), std::nullopt, std::nullopt));
                             } else {
                                 ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 3, message,
                                                   app_meta, api, "Mandatory information element missing", ex.what(), std::nullopt, std::nullopt));
                             }
                             return true;
                        }

                        if (!validate_state_setting_options(user_data_ing_session, stream, message, app_meta, api)) return true;

                        try {
                            App::self().context()->addUserDataIngSession(user_data_ing_session);
                            user_data_ing_session->processDistributionSessionInfo(stream_id, request_ctx);
                        } catch (std::out_of_range &ex) {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 3, message,
                                                    app_meta, api, "MBS User Service does not exist", ex.what(), std::nullopt,
                                                    std::nullopt));
                        }

                        return true;
                    } else if (method == OGS_SBI_HTTP_METHOD_GET) {
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

                        try {
                            std::shared_ptr<UserDataIngSession> user_data_ing_sess = find(user_data_ing_session_id);
                            user_data_ing_sess->processUserDataIngSessionUpdate(stream_id, request_ctx, user_data_ing_sess_update);

                            int response_code = 200;
                            CJson user_data_ing_session_json(user_data_ing_sess->json(false));
                            std::string body(user_data_ing_session_json.serialise());
                            ogs_debug("Parsed JSON: %s", body.c_str());
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
                        }

                        return true;

                    } else if (method == OGS_SBI_HTTP_METHOD_PATCH) {

                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                            app_meta, api, "Method not allowed",
                                                            "The PATCH method is not allowed for this path"));

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
                             std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, OGS_SBI_HTTP_METHOD_POST ", " OGS_SBI_HTTP_METHOD_GET ", " OGS_SBI_HTTP_METHOD_DELETE ", " OGS_SBI_HTTP_METHOD_OPTIONS, api, app_meta));
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
            std::shared_ptr<UserDataIngDistSessId> ids_ptr = std::make_shared<UserDataIngDistSessId>(*ids);

            try {
                std::shared_ptr<UserDataIngSession> ing_session = find(ids_ptr->first);
                ing_session->nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSession, event.sbiData(), event.sbiData());
                return true;
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids_ptr->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            return true;

        }
        case MBSF_LOCAL_SEND_MBSTF_PATCH_BUILD:
        {
            SessionIdContainer *ids = reinterpret_cast<SessionIdContainer*>(event.sbiData());
            try {
                std::shared_ptr<UserDataIngSession> ing_session = find(ids->second->first);
                sendMbsmfActivityStatus(ids->second);
                ing_session->nmbstfDiscoverAndSend(ids->second, Nmb2Build::buildNmb2DistSessionPatch, nullptr, ids);
                return true;
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->second->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            return true;

        }

        case MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION:
        {
            SessionIdContainer *ids = reinterpret_cast<SessionIdContainer*>(event.sbiData());
            try {
                std::shared_ptr<UserDataIngSession> ing_session = find(ids->second->first);
                std::shared_ptr<ContextData> context_data = ing_session->getDistributionSessionInfoData(ids->second->second);
                context_data->MBSSession->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_INACTIVE);
                context_data->MBSSession->pushChanges();
                ing_session->nmbstfDiscoverAndSend(ids->second, Nmb2Build::buildNmb2DistSessionDelete, nullptr, ids);
                return true;
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->second->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            return true;
        }
        case MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK:
        {
            SessionIdContainer *ids = reinterpret_cast<SessionIdContainer*>(event.sbiData());
            try {
                std::shared_ptr<UserDataIngSession> ing_session = find(ids->second->first);

                ing_session->nmbstfDiscoverAndSend(ids->second, /*Nmb2Build::buildNmb2DistSessionStatePatch*/Nmb2Build::buildNmb2DistSessionPatch, nullptr, ids);
                return true;
            } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids->second->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
            }
            return true;
        }

        default:
            return false;
    }
    return false;
}

ogs_sbi_xact_t *UserDataIngSession::nmbstfDiscoverOnly(std::shared_ptr< ContextData > data)
{
    ogs_sbi_xact_t *xact = nullptr;

    xact = m_sbiObject->discoverOnly(data->streamId, OGS_SBI_SERVICE_TYPE_NMBSTF_DISTSESSION, nullptr);
    if (!xact) {
        ogs_error("discoverOnly() failed");
    } else {
       std::shared_ptr<UserDataIngDistSessId> ids = nullptr;
       ids.reset(new UserDataIngDistSessId(data->ingSessionId, data->distSessionInfoKey));
       addToRegistry(xact, ids);
    }
    return xact;
}

ogs_sbi_xact_t *UserDataIngSession::nmbstfDiscoverAndSend(std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids, ogs_sbi_build_f build, void *context, void *data)
{
    ogs_sbi_xact_t *xact = nullptr;
    ogs_sbi_discovery_option_t *discovery_option = nullptr;

    std::shared_ptr<ContextData> context_data = getContextData(ids);
    if (!context_data->mbstfNFInstanceId.empty()) {
        discovery_option = ogs_sbi_discovery_option_new();
        ogs_assert(discovery_option);
        ogs_sbi_discovery_option_set_target_nf_instance_id(discovery_option, const_cast<char*>(context_data->mbstfNFInstanceId.c_str()));
    }
    xact = m_sbiObject->discoverAndSend(context_data->streamId, OGS_SBI_SERVICE_TYPE_NMBSTF_DISTSESSION, discovery_option, build, context, data);
    if (!xact) {
        ogs_error("discoverAndSend() failed");
    } else {
       addToRegistry(xact, ids);
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

std::shared_ptr< DistSessionState > UserDataIngSession::getDistSessionState()
{
   std::shared_ptr< DistSessionState > dist_session_state(new DistSessionState());
   if (m_activePeriods) {

       DistSessionState dist_sess_state = m_activePeriods->currentState(std::nullopt);
       *dist_session_state = dist_sess_state.getValue();
   }

   if (m_activePeriodsRepRule) {
       DistSessionState dist_sess_state = m_activePeriodsRepRule->currentState(std::nullopt);
       *dist_session_state = dist_sess_state.getValue();
   }

   if (m_alwaysActive) {

       DistSessionState dist_sess_state = m_alwaysActive->currentState(std::nullopt);
       *dist_session_state = dist_sess_state.getValue();
   }

   /*
   {
       std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
       m_distSessionState = *dist_session_state;
   }
   */
   return dist_session_state;

}

const DistSessionState UserDataIngSession::getNextDistSessionState() const
{
    ActivePeriodsBase::TimestampAndActiveFlag transition;

    if (m_activePeriods) {

        transition = m_activePeriods->nextTransition();
    } else if (m_activePeriodsRepRule) {

        transition = m_activePeriodsRepRule->nextTransition();
    } else if (m_alwaysActive) {

        transition = m_alwaysActive->nextTransition();
    }
    return transition.second;

}


 const std::string &UserDataIngSession::distSessionState() const
{
    {
       std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
       return m_distSessionState;
   }

};

const DistSessionState UserDataIngSession::getdistSessState() const
{
    {
       std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
       return m_distSessionState;
   }
}

void UserDataIngSession::sendMbsmfActivityStatus(std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > user_data_ing_dist_sess_ids)
{
    std::shared_ptr<ContextData> context_data = getContextData(user_data_ing_dist_sess_ids);

    std::optional<std::shared_ptr< DistSessionState > > dist_session_state = context_data->info->getMbsDistSessState();
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

void UserDataIngSession::sendNotificationsEvent(std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > user_data_ing_dist_sess_ids)
{
    std::shared_ptr< UserDataIngSession::ContextData > context_data = getContextData(user_data_ing_dist_sess_ids);

    std::optional<std::shared_ptr< DistSessionState > > dist_session_state = context_data->info->getMbsDistSessState();
    if(!dist_session_state.has_value()) return;

    std::shared_ptr< DistSessionState > dist_sess_state = dist_session_state.value();

    if(*dist_sess_state == DistSessionState::VAL_ESTABLISHED) {
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
    ActivePeriodsBase::TimestampAndActiveFlag transition;

    if (!m_startTimer) return false;

    if (m_activePeriods) {

        transition = m_activePeriods->nextTransition();
    } else if (m_activePeriodsRepRule) {

        transition = m_activePeriodsRepRule->nextTransition();
    } else if (m_alwaysActive) {

        transition = m_alwaysActive->nextTransition();
        m_distSessionState = transition.second;
    }

    if (transition.first.has_value()) {

        {
            std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
            m_distSessionState = transition.second;
        }
        std::int64_t dur = duration_timer(transition.first.value());
        ogs_time_t duration = dur;

        m_activePeriodsTimer->stop();
        m_activePeriodsTimer->start(duration);
        //ogs_timer_start(m_activePeriodsTimer, ogs_time_from_msec(duration));
        return true;
    }
    return false;

}

std::map<std::string, std::shared_ptr< UserDataIngSession::ContextData >> &UserDataIngSession::distributionSessionInfos() {
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    return m_distributionSessionInfos;
};


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

std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::getDistributionSessionInfoData(const std::string &key)
{

    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    auto it = m_distributionSessionInfos.find(key);
    if (it != m_distributionSessionInfos.end()) {
        return it->second;
    }
    return nullptr;

}

void UserDataIngSession::removeDistributionSessionInfo(std::string &key)
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

void UserDataIngSession::deleteDistributionSessionInfo(std::string &key)
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    m_distributionSessionInfos.erase(key);
}

void UserDataIngSession::addToRegistry(ogs_sbi_xact_t* xact, std::shared_ptr< UserDataIngDistSessId > &ids)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    s_xactRegistry[xact] = ids;

}

void UserDataIngSession::addToRegistry(std::string dist_session_id, std::shared_ptr< UserDataIngDistSessId > &ids)
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

void UserDataIngSession::removeFromRegistry(std::string &dist_session_id)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    s_distSessionIdRegistry.erase(dist_session_id);
}


std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > UserDataIngSession::getFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<decltype(s_registry_mutex)> lock(s_registry_mutex);
    auto it = s_xactRegistry.find(xact);
    if (it != s_xactRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > UserDataIngSession::getFromRegistry(std::string &dist_session_id)
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
    char *id = (char *)data;
    std::string user_data_ing_session_id = std::string(id);
    try {
        std::shared_ptr<UserDataIngSession> user_data_ing_sess = find(user_data_ing_session_id);

        /*
        if (!user_data_ing_sess->checkIfAllMBSTFResponsesReceived()) {
        reset_timer(ids);
        return;
        }
        */

        for (auto &[dist_sess_id, user_ing_sess_id_ptr] : s_distSessionIdRegistry) {
            if (user_ing_sess_id_ptr->first == user_data_ing_session_id) {
                std::shared_ptr<DistSessionState> dist_sess_state = nullptr;
                SessionIdContainer *session_id = new SessionIdContainer(dist_sess_id, user_ing_sess_id_ptr);
                //UserDataIngDistSessId *ids =  new UserDataIngDistSessId(user_ing_sess_id_ptr->first, user_ing_sess_id_ptr->second);
                std::shared_ptr<UserDataIngDistSessId> ids_ptr(user_ing_sess_id_ptr);
                std::shared_ptr<ContextData> context_data = getContextData(ids_ptr);
                context_data->stateUpdate = true;
                dist_sess_state.reset(new DistSessionState());
                *dist_sess_state = user_data_ing_sess->distSessionState();
                context_data->info->setMbsDistSessState(dist_sess_state);
                sendMbsmfActivityStatus(ids_ptr);
		UserDataIngSession::sendNotificationsEvent(ids_ptr);

                user_data_ing_sess->nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSessionPatch, nullptr, session_id);
            }
        }
        user_data_ing_sess->startTimer();
    }  catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << user_data_ing_session_id << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::handleUserDataIngSessionUpdate(ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request)
{
    std::shared_ptr<ContextData> ctx_data = nullptr;
    std::shared_ptr<DistSessionState> dist_sess_state = nullptr;

    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();

    setMbstfsInDesiredState();

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
                        std::optional<std::shared_ptr< Ssm > > ssm = mbs_sess_id->getSsm();
                        if (ssm.has_value()) {
                            std::shared_ptr< Ssm > ssm_val = *ssm;
                            std::shared_ptr< IpAddr > src_ip_addr = ssm_val->getSourceIpAddr();
                            std::shared_ptr< IpAddr > dest_ip_addr = ssm_val->getDestIpAddr();
                            std::optional<std::string > src_ipv4_addr = src_ip_addr->getIpv4Addr();
                            std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
                            std::optional<std::shared_ptr< std::string > > src_ipv6_addr = src_ip_addr->getIpv6Addr();
                            std::optional<std::shared_ptr< std::string > > dest_ipv6_addr = dest_ip_addr->getIpv6Addr();
                            std::shared_ptr< Ssm > ssm_data = nullptr;

                            ssm_data.reset(new Ssm(*ssm_val));
                            if (dest_ipv4_addr.has_value() || dest_ipv6_addr.has_value()) {
                                distribution_session_info.reset(new DistributionSessionInfo(info));
                                ctx_data.reset(new ContextData{m_UserDataIngSessionId, key, distribution_session_info, info, ssm_data, request, stream_id, nullptr});
                                addToDistributionSessionInfos(key, ctx_data);
                                nmbstfDiscoverOnly(ctx_data);

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
    updateMbstfRemovedDistSession();
    startTimer();

}

void UserDataIngSession::processUserDataIngSessionUpdate(ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request, CJson &json)
{

    bool always_active = true;
    std::shared_ptr< DistSessionState > dist_sess_state = nullptr;


    std::shared_ptr<MBSUserDataIngSession> mbs_user_data_ing_session = nullptr;
    mbs_user_data_ing_session.reset(new MBSUserDataIngSession(json, true));
    const ActPeriodsType &act_periods = mbs_user_data_ing_session->getActPeriods();
    const ActPeriodsType &current_act_periods = m_MBSUserDataIngSession->getActPeriods();

    if (current_act_periods.has_value()) {
        m_MBSUserDataIngSession->clearActPeriods();
    }

    if (act_periods.has_value() && !act_periods->empty()) {
        m_MBSUserDataIngSession->setActPeriods(std::move(act_periods));
        always_active = false;

        resetAlwaysActive();
        resetActivePeriodsRepRule();
        activePeriods(act_periods);
        dist_sess_state = getDistSessionState();
        createTimer();
    }

    m_MBSUserDataIngSession->setActPeriodsRepRule(std::nullopt);

    const ActPeriodsRepRuleType &act_periods_rep_rule = mbs_user_data_ing_session->getActPeriodsRepRule();

    if (act_periods_rep_rule.has_value()) {
        m_MBSUserDataIngSession->setActPeriodsRepRule(std::move(act_periods_rep_rule));
        always_active = false;

        resetAlwaysActive();
        resetActivePeriods();

        activePeriodsRepRule(act_periods_rep_rule);
        dist_sess_state = getDistSessionState();
        createTimer();
    }

    auto app_context = App::self().context();
    const MBSUserDataIngSession::MbsDisSessInfosType  current_dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();
    for(const auto &[key, sess_info]: current_dist_sess_infos) {

        std::shared_ptr<ContextData> context_data = getDistributionSessionInfoData(key);
        ogs_assert(context_data);
        bool present_in_update = false;
        if (sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = sess_info.value();
            const MBSUserDataIngSession::MbsDisSessInfosType &update_dist_sess_infos = mbs_user_data_ing_session->getMbsDisSessInfos();
            for(const auto &[key_in_update, sess_info_update]: update_dist_sess_infos) {
                if (!sess_info_update.has_value() /*|| sess_info_update.value() == nullptr */) {
                    continue;
                }
                std::shared_ptr<MBSDistributionSessionInfo> update_info = sess_info_update.value();

                if (key == key_in_update) {
                    present_in_update = true;
                    std::shared_ptr<MBSDistributionSessionInfo> update_info = sess_info_update.value();
                    update_info->setMbsDistSessionId(info->getMbsDistSessionId());
                    //update_info->setMbsDistSessState(info->getMbsDistSessState());
                    if (!info && !update_info) {

                        context_data->needsUpdate = false;
                        continue;
                    } else if ( info && update_info && info == update_info) {
                        context_data->needsUpdate = false;
                        continue;
                    } else if (info && update_info && *info == *update_info) {
                        context_data->needsUpdate = false;
                        continue;
                    } else {

                        //change_mbs_dist_session_infos(context_data, info, update_info);

                        context_data->distributionSessionInfo->updateMBSDistributionSessionInfo(update_info);
                        context_data->needsUpdate = true;
                    }

                } else {
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
                std::optional<std::shared_ptr< DistSessionState > > received_dist_session_state = update_info->getMbsDistSessState();
                if (!received_dist_session_state.has_value()) {
                    if (always_active) {
                        resetActivePeriods();
                        resetActivePeriodsRepRule();
                        alwaysActive();

                        dist_sess_state = getDistSessionState();
                        if (dist_sess_state) info->setMbsDistSessState(dist_sess_state);
                    } else {

                        if (dist_sess_state) info->setMbsDistSessState(dist_sess_state);
                    }
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

    }

    const std::optional<std::shared_ptr< reftools::mbsf::MBSUserServAnmt > >  &mbs_user_serv_anmt = mbs_user_data_ing_session->getMbsUserServAnmt();
    if (mbs_user_serv_anmt.has_value() && mbs_user_serv_anmt.value()) {
        m_MBSUserDataIngSession->setMbsUserServAnmt(std::move(mbs_user_serv_anmt));
    }

    const std::optional<std::shared_ptr<  reftools::mbsf::UserServiceDescription > > user_service_description =  mbs_user_data_ing_session->getMbsUserServiceAnmt();
    if (user_service_description.has_value() && mbs_user_serv_anmt.value()) {
        m_MBSUserDataIngSession->setMbsUserServiceAnmt(std::move(user_service_description));
    }

    const std::optional<std::string > serv_anmt_url = mbs_user_data_ing_session->getMbsUserServiceAnmtUrl();
    if (serv_anmt_url.has_value() && !serv_anmt_url.value().empty()) {
        m_MBSUserDataIngSession->setMbsUserServiceAnmtUrl(std::move(serv_anmt_url));
    }
    handleUserDataIngSessionUpdate(stream_id, request);
}

void UserDataIngSession::processDistributionSessionInfo( ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request)
{
    bool always_active = true;
    std::shared_ptr<ContextData> ctx_data = nullptr;
    std::shared_ptr<DistSessionState> dist_sess_state = nullptr;

    const ActPeriodsType &act_periods =  m_MBSUserDataIngSession->getActPeriods();
    const ActPeriodsRepRuleType &act_periods_rep_rule = m_MBSUserDataIngSession->getActPeriodsRepRule();

    if (act_periods.has_value() && !act_periods->empty()) {
        always_active = false;

        resetAlwaysActive();
        resetActivePeriodsRepRule();

        activePeriods(act_periods);
        dist_sess_state = getDistSessionState();
        createTimer();
    }

    if (act_periods_rep_rule.has_value()) {
        always_active = false;

        resetAlwaysActive();
        resetActivePeriods();

        activePeriodsRepRule(act_periods_rep_rule);
        dist_sess_state = getDistSessionState();
        createTimer();
    }

    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();

    for (const auto &[key, sess_info]: dist_sess_infos) {
        if (sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = sess_info.value();
            if (info) {
                std::shared_ptr<DistributionSessionInfo> distribution_session_info = nullptr;

                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if (mbs_session_id.has_value()) {
                    std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                    std::optional<std::shared_ptr< Ssm > > ssm = mbs_sess_id->getSsm();
                    if (ssm.has_value()) {
                        std::shared_ptr< Ssm > ssm_val = *ssm;
                        //std::shared_ptr< IpAddr > src_ip_addr = ssm_val->getSourceIpAddr();
                        std::shared_ptr< IpAddr > dest_ip_addr = ssm_val->getDestIpAddr();
                        //auto &src_ipv4_addr = src_ip_addr->getIpv4Addr();
                        auto &dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
                        //auto &src_ipv6_addr = src_ip_addr->getIpv6Addr();
                        auto &dest_ipv6_addr = dest_ip_addr->getIpv6Addr();
                        std::shared_ptr< Ssm > ssm_data = nullptr;

                        ssm_data.reset(new Ssm(*ssm_val));
                        if (dest_ipv4_addr.has_value() || dest_ipv6_addr.has_value()) {
                            distribution_session_info.reset(new DistributionSessionInfo(info));
                            ctx_data.reset(new ContextData{std::string(m_UserDataIngSessionId), std::string(key), distribution_session_info, info, ssm_data, request, stream_id, nullptr});
                            addToDistributionSessionInfos(std::string(key), ctx_data);
                            nmbstfDiscoverOnly(ctx_data);

                        } else {
                            ogs_error("Unable to resolve SSM addresses");
                            continue;
                        }

                    }

                }
                std::optional<std::shared_ptr< DistSessionState > > received_dist_session_state = info->getMbsDistSessState();
                if (!received_dist_session_state.has_value()) {

                    if (always_active) {
                        resetActivePeriods();
                        resetActivePeriodsRepRule();
                        alwaysActive();
                        dist_sess_state = getDistSessionState();
                        if (dist_sess_state) info->setMbsDistSessState(dist_sess_state);
                        //ActivePeriodsBase::TimestampAndActiveFlag transition = always_active->nextTransition();
                    } else {
                        if (dist_sess_state) info->setMbsDistSessState(dist_sess_state);
                    }
                }
            }
        }
    }
    startTimer();
}

bool UserDataIngSession::processDistSession(std::shared_ptr< DistSession > dist_session)
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

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
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

std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::setDistSessionId( std::shared_ptr< UserDataIngSession::ContextData > context_data, std::string dist_session_id)
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
    //DistSessionState current_dist_session_state(std::move(m_currentDistSessionState));
    std::shared_ptr< DistSessionState > dist_session_state = getDistSessionState();
    DistSessionState dist_sess_state = getNextDistSessionState();
    std::string desired_state = getDistSessionState()->getString();

    if (m_currentDistSessionState == DistSessionState::VAL_ACTIVE && (*dist_session_state == DistSessionState::VAL_ESTABLISHED || dist_sess_state == DistSessionState::VAL_ESTABLISHED))
    {
        m_startTimer = false;
        m_activePeriodsTimer->stop();
        m_distSessionState = DistSessionState::VAL_INACTIVE;
        m_desiredDistSessionState = DistSessionState::VAL_ESTABLISHED;
        changeDistSessionState((void *)m_UserDataIngSessionId.c_str());
    }
}

void UserDataIngSession::checkDesiredState()
{
    if (m_desiredDistSessionState != DistSessionState::NO_VAL) {
        m_startTimer = false;
        m_activePeriodsTimer->stop();
        m_distSessionState = m_desiredDistSessionState;
        changeDistSessionState((void *)m_UserDataIngSessionId.c_str());
        m_currentDistSessionState = DistSessionState::NO_VAL;
        m_desiredDistSessionState = DistSessionState::NO_VAL;
    } else {
        m_startTimer = true;
        startTimer();
    }
}

bool UserDataIngSession::sendNmbsfMbsUserDataIngestResponse(std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids)
{

    std::shared_ptr<ContextData> context_data = getContextData(ids);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);

        std::shared_ptr<Open5GSSBIRequest> request = context_data->request;
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

    Open5GSSBIClient mbstf_client(reinterpret_cast<ogs_sbi_client_t*>(NF_INSTANCE_CLIENT(nf_instance)));
    std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session;
    std::string src_ipv4_addr;
    std::string src_ipv6_addr;

    ogs_sockaddr_t *client_ipv4_addr =  mbstf_client.ogsSBIClientIPv4Addr();
    ogs_sockaddr_t *client_ipv6_addr =  mbstf_client.ogsSBIClientIPv6Addr();

    if (client_ipv4_addr) {
        char *ipv4_addr = mbstf_client.ogsIpStrdup(client_ipv4_addr);
        src_ipv4_addr = std::string(ipv4_addr);
        ogs_free(ipv4_addr);
    }

    if (client_ipv6_addr) {
         char *ipv6_addr = mbstf_client.ogsIpStrdup(client_ipv6_addr);
         src_ipv6_addr = std::string(ipv6_addr);
         ogs_free(ipv6_addr);
    }


    std::shared_ptr<ContextData> context_data = nullptr;
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

    std::shared_ptr<UserDataIngSession> ing_session = find(ids->first);
    if (nf_instance->t_validity)  ogs_timer_stop(nf_instance->t_validity);
    ing_session->setNFInstance(xact->service_type, nf_instance);


    context_data = getContextData(ids);
    if (!context_data) {
        ogs_error("Unable to get context data from registry");
        return false;
    }

    if (context_data->mbstfNFInstanceId.empty()) context_data->mbstfNFInstanceId = std::string(nf_instance->id);

    std::shared_ptr<Ssm> ssm_ptr = context_data->ssm;
    if (!ssm_ptr) ogs_error("Unable to get SSM from Context Data");

    std::shared_ptr< IpAddr > dest_ip_addr = ssm_ptr->getDestIpAddr();
    std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
    std::optional<std::shared_ptr< std::string > > dest_ipv6_addr = dest_ip_addr->getIpv6Addr();

    if (!client_ipv4_addr && !client_ipv6_addr) {
        mb_smf_mbs_session.reset(new MBSMFMBSSession(mb_smf_sc_mbs_session_new()));
        mb_smf_mbs_session->setTunnelRequest(true);

    } else if (client_ipv4_addr && dest_ipv4_addr.has_value()) {
        struct addrinfo *ai_src = NULL, *ai_dest = NULL;
        void *src_addr = NULL, *dest_addr = NULL;

        if (resolve_src_dest_addr(src_ipv4_addr, dest_ipv4_addr.value(), &ai_src, &ai_dest))
        {
            if (get_src_dest_of_same_addr_family(AF_INET, ai_src, ai_dest, &src_addr, &dest_addr))
            {
                mb_smf_mbs_session.reset(new MBSMFMBSSession(
                mb_smf_sc_mbs_session_new_ipv4((const struct in_addr*)src_addr,
                                                     (const struct in_addr*)dest_addr)));

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
        }
        if (ai_src) {
            freeaddrinfo(ai_src);
            ai_src = NULL;
        }

        if (ai_dest) {
            freeaddrinfo(ai_dest);
            ai_dest = NULL;
        }

    } else if (client_ipv6_addr && dest_ipv6_addr.has_value()) {
       struct addrinfo *ai_src = NULL, *ai_dest = NULL;
       void *src_addr = NULL, *dest_addr = NULL;
       std::shared_ptr< std::string >  dest_ipv6 = dest_ipv6_addr.value();

       if (resolve_src_dest_addr(src_ipv6_addr, *dest_ipv6, &ai_src, &ai_dest))
       {
           if (get_src_dest_of_same_addr_family(AF_INET6, ai_src, ai_dest, &src_addr, &dest_addr))
           {
               mb_smf_mbs_session.reset(new MBSMFMBSSession(
               mb_smf_sc_mbs_session_new_ipv6((const struct in6_addr*)src_addr,
                               (const struct in6_addr*)dest_addr)));

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv6 address family");
               if (ai_src) freeaddrinfo(ai_src);
               if (ai_dest) freeaddrinfo(ai_dest);
               return false;
           }
       }
       if (ai_src) freeaddrinfo(ai_src);
       if (ai_dest) freeaddrinfo(ai_dest);

    } else {
       ogs_error("Unable to resolve SSM addresses");
       return false;
    }

    if (mb_smf_mbs_session->ssm()) {

        mb_smf_mbs_session->setTunnelRequest(true);
        mb_smf_mbs_session->setTmgiRequest(true);

        mb_smf_mbs_session->setServiceType(MBS_SERVICE_TYPE_MULTICAST);
        if (!context_data->MBSSession) context_data->MBSSession = mb_smf_mbs_session;
        UserDataIngDistSessId *ids = new UserDataIngDistSessId(context_data->ingSessionId, context_data->distSessionInfoKey);
        mb_smf_mbs_session->setCreatedCallback(reinterpret_cast<void*>(ids) /*(context_data.get()*/);
        populate_mb_smf_mbs_session(context_data, mb_smf_mbs_session);

        /*
        if ((context_data->info->getMbsDistSessState() == DistSessionState::VAL_ACTIVE ) ||
                (context_data->info->getMbsDistSessState() == DistSessionState::VAL_ESTABLISHED)) {
            mb_smf_mbs_session->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_ACTIVE);
        } else {

            mb_smf_mbs_session->setActivityStatus(MBS_SESSION_ACTIVITY_STATUS_INACTIVE);
        }

        context_data->MBSSession = mb_smf_mbs_session;

        mb_smf_mbs_session->pushChanges();
        */
    } else {
        ogs_info("MB-SMF SSM is not present");
    }

    removeFromRegistry(xact);

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

void UserDataIngSession::handlePatchUpdateResponse(ogs_sbi_xact_t *xact)
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
        std::shared_ptr<UserDataIngSession> ing_session = find(ids->first);
        std::shared_ptr<ContextData> context_data = getContextData(ids);
        ing_session->checkDesiredState();
        context_data->receivedMBSTFPatchResponse = true;
        context_data->patchUpdateSucceded = true;
        context_data->needsUpdate = false;
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }

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
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);

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


std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::getContextData(std::shared_ptr<UserDataIngDistSessId> &ids)
{
    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        return ing_sess->getDistributionSessionInfoData(ids->second);
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
    return nullptr;

}


void UserDataIngSession::removeDistributionSessionInfos(void *data)
{

    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    try{
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        // Do something here?
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
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

        sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_REQ_BUILD, reinterpret_cast<void *>(ids));

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

void UserDataIngSession::setMBSSessionFlag(void *data)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids->second);
        context_data->MBSSessionStatus = MBSSessionState::CREATED;
        if (ing_sess->checkIfAllMBSSessionResponsesReceived()) {
            bool rv = ing_sess->checkIfAllMBSSessionCreated();
            if (!rv) {
                ing_sess->handleFailedMBSSession();
            }

        }
        //ing_sess->checkIfAllMBSSessionCreated();
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::setMBSSessionDeleted(void *data)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids->second);
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
                removeFromRegistry(ids->second);
                ing_sess->removeDistributionSessionInfo(ids->first);
            }
            App::self().context()->deleteUserDataIngSession(ing_sess->m_UserDataIngSessionId);
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    } 
    delete ids;
}

void UserDataIngSession::setMBSSessionFailureFlag(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
        std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids->second);
        context_data->MBSSessionStatus = MBSSessionState::FAILED;
        //context_data->hasMBSSession = true;
        if (ing_sess->checkIfAllMBSSessionResponsesReceived()) {
            std::string user_data_ingest_session_id(ids->first);
            populateAndSendError(data, cause, problem_detail_json);
            App::self().context()->deleteUserDataIngSession(user_data_ingest_session_id);
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }


}

void UserDataIngSession::handleFailedMBSSession()
{
    std::lock_guard<decltype(m_distSessInfosMutex)::element_type> lock(*m_distSessInfosMutex);
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus == MBSSessionState::FAILED) {
            UserDataIngDistSessId *ids = new UserDataIngDistSessId(dist_sess_info.second->ingSessionId, dist_sess_info.second->distSessionInfoKey);
            populateAndSendError(reinterpret_cast<void*>(ids), dist_sess_info.second->mbsmfProblemCause, dist_sess_info.second->mbsmfProblemDetailJson);
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

void UserDataIngSession::setMBSTFDistSessionDeletedFlag(std::string &dist_session_id)
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
        //if (context_data->markForDeletion) {
            //removeFromRegistry(dist_session_id);
            //ing_sess->removeDistributionSessionInfo(ids->first);
            //return;
        //}
    }
    //if (ing_sess->checkIfAllMBSTFDistSessionDeleted()) {
    //    App::self().context()->deleteUserDataIngSession(ing_sess->m_UserDataIngSessionId);
    //}
}

void UserDataIngSession::handleMBSSessionError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);
    std::string user_data_ingest_session_id(ids->first);
    populateAndSendError(data, cause, problem_detail_json);
    App::self().context()->deleteUserDataIngSession(user_data_ingest_session_id);;

}

void UserDataIngSession::populateAndSendError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{

    std::shared_ptr<UserDataIngDistSessId> ids_ptr(reinterpret_cast<UserDataIngDistSessId*>(data));
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

    removeDistributionSessionInfos(data);

    std::ostringstream err;

    std::string error = print_mbs_session_error(context_data);

    if (cause.has_value()) {
        ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, cause.value(), error.c_str()));

    } else {

        ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, ProblemCause::INBOUND_SERVER_ERROR, error.c_str()));

    }
}


bool UserDataIngSession::tmgi(mb_smf_sc_tmgi_t *tmgi, void *data)
{
    bool tmgi_set = false;

    //const char *tmgi_repr = mb_smf_sc_tmgi_repr(tmgi);
    //char *plmn_id = ogs_plmn_id_to_string(&tmgi->plmn, buf);
    char *mcc = ogs_plmn_id_mcc_string(&tmgi->plmn);
    char *mnc = ogs_plmn_id_mnc_string(&tmgi->plmn);

    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);
    std::shared_ptr<UserDataIngSession> ing_sess = find(ids->first);
    std::shared_ptr<ContextData> context_data = ing_sess->getDistributionSessionInfoData(ids->second);
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


std::shared_ptr< ObjDistributionOperatingMode > UserDataIngSession::getOperatingMode(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getOperatingMode();
    }
    return nullptr;
}

std::shared_ptr< ObjAcquisitionMethod > UserDataIngSession::getAcquisitionMethod(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqMethod();
    }
    return nullptr;
}

std::optional<std::string> UserDataIngSession::getObjectIngestUrl(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjIngUri();
    }
    return std::nullopt;
}

std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > UserDataIngSession::getObjectAcquisitionIds(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqIds();
    }
    return {};
}

std::optional<std::string> UserDataIngSession::getObjectDistributionUrl(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjDistrUri();
    }
    return std::nullopt;
}

std::optional<std::shared_ptr< PacketDistrMethInfo > > UserDataIngSession::getPktDistributionInfo(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    return info->getPckDistrInfo();
}

std::shared_ptr< PktDistributionOperatingMode > UserDataIngSession::getPktDistributionOperatingMode(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< PacketDistrMethInfo > > pkt_dist_method_info = info->getPckDistrInfo();
    if (pkt_dist_method_info.has_value()) {
        std::shared_ptr< PacketDistrMethInfo > dist_method_info = pkt_dist_method_info.value();
        return dist_method_info->getOperatingMode();
    }
    return nullptr;
}

std::shared_ptr< PktIngestMethod > UserDataIngSession::getPktIngestMethod(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< PacketDistrMethInfo > > pkt_dist_method_info = info->getPckDistrInfo();
    if (pkt_dist_method_info.has_value()) {
        std::shared_ptr< PacketDistrMethInfo > dist_method_info = pkt_dist_method_info.value();
        return dist_method_info->getPckIngMethod();
    }
    return nullptr;
}

std::shared_ptr< MbStfIngestAddr > UserDataIngSession::getMbstfIngestAddr(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< PacketDistrMethInfo > > pkt_dist_method_info = info->getPckDistrInfo();
    if (pkt_dist_method_info.has_value()) {
        std::shared_ptr< PacketDistrMethInfo > dist_method_info = pkt_dist_method_info.value();
        return dist_method_info->getIngEndpointAddrs();
    }
    return nullptr;
}

std::optional <std::string > UserDataIngSession::getTrafficMarkingInfo(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    return info->getTrafficMarkingInfo();
}



std::string UserDataIngSession::maxContBitRate(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    return info->getMaxContBitRate();
}

void UserDataIngSession::pendingDeleteResponse(ogs_pool_id_t stream_id)
{
    std::lock_guard<decltype(m_deleteRequestsMutex)::element_type> lock(*m_deleteRequestsMutex);
    m_deleteRequests.push_back(stream_id);
}

static void process_mbs_distribution_session_info(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::shared_ptr< DistSession > dist_session)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > >  obj_distribution_method_info = context_data->info->getObjDistrInfo();
    if (obj_distribution_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > obj_dist_method_info = obj_distribution_method_info.value();
        if (obj_dist_method_info->getOperatingMode()->getString() == "PUSH") {
            std::optional<std::shared_ptr< ObjDistributionData > > obj_distribution_data = dist_session->getObjDistributionData();
            if (obj_distribution_data.has_value()) {
                std::shared_ptr< ObjDistributionData > obj_dist_data = obj_distribution_data.value();
                std::shared_ptr< ObjAcquisitionMethod> obj_acquisition_method = obj_dist_data->getObjAcquisitionMethod();
                      if (obj_acquisition_method->getString() == "PUSH") {
                    obj_dist_method_info->setObjIngUri(obj_dist_data->getObjIngestBaseUrl());
                }

            }

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

static std::string print_mbs_session_error(
    std::shared_ptr<UserDataIngSession::ContextData> context_data
) {
    auto ssm_val      = context_data->ssm;
    auto src_ip_addr  = ssm_val->getSourceIpAddr();
    auto dest_ip_addr = ssm_val->getDestIpAddr();

    std::optional<std::string> src_ipv4_addr  = src_ip_addr->getIpv4Addr();
    std::optional<std::string> dest_ipv4_addr = dest_ip_addr->getIpv4Addr();

    std::optional<std::shared_ptr<std::string>> src_ipv6_ptr  = src_ip_addr->getIpv6Addr();
    std::optional<std::shared_ptr<std::string>> dest_ipv6_ptr = dest_ip_addr->getIpv6Addr();

    std::optional<std::string> src_ipv6_addr;
    if (src_ipv6_ptr && *src_ipv6_ptr) {
        src_ipv6_addr = **src_ipv6_ptr;
    }

    std::optional<std::string> dest_ipv6_addr;
    if (dest_ipv6_ptr && *dest_ipv6_ptr) {
        dest_ipv6_addr = **dest_ipv6_ptr;
    }

    const char* tmgi_cstr = context_data->MBSSession->tmgi();
    ogs_debug("TMGI Error: %s", tmgi_cstr);
    std::optional<std::string> tmgi_opt;
    if (tmgi_cstr && *tmgi_cstr) {
        tmgi_opt = tmgi_cstr;
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

    oss << "] already exists in the MBS System\n";
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

static std::int64_t duration_timer(const std::chrono::system_clock::time_point  &tp) {
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(tp - now).count();
    if (diff <= 0) return static_cast<std::int64_t>(1);
    return static_cast<std::int64_t>(diff);
}

static bool validate_state_setting_options(std::shared_ptr<UserDataIngSession> user_data_ing_session, Open5GSSBIStream &stream, Open5GSSBIMessage &message, const NfServer::AppMetadata &app_meta, std::optional<NfServer::InterfaceMetadata> api)
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
                if (mbs_session_id) {
                    const auto &mbs_service_area = info->getTgtServAreas();
                    const auto &ext_mbs_service_area = info->getExtTgtServAreas();
                    UniqueMbsSessionId unique_mbs_session_id(!!mbs_session_id.value()->getSsm(), mbs_session_id.value(),
                                    mbs_service_area?mbs_service_area.value():std::shared_ptr<MbsServiceArea>(),
                                    ext_mbs_service_area?ext_mbs_service_area.value():std::shared_ptr<ExternalMbsServiceArea>());
                    if (context->haveMbsSessionId(unique_mbs_session_id)) {
                        invalid_params[std::format("mbsDisSessInfos.{}.mbsSessionId", dist_sess_id)] = "mbsSessionId already used in another UserDataIngSession";
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

static std::shared_ptr<MBSMFMBSSession> populate_mb_smf_mbs_session(std::shared_ptr< UserDataIngSession::ContextData > context_data,
                std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session) {

    std::optional<std::shared_ptr< DistSessionState > > dist_session_state = context_data->info->getMbsDistSessState();
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
                                                   std::optional<NfServer::InterfaceMetadata> api,
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

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

