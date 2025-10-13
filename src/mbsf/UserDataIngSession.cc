/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Session class
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

// standard template library includes
#include <chrono>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

// App header includes
#include "common.hh"
#include "AlwaysActive.hh"
#include "ActivePeriods.hh"
#include "ActivePeriodsBase.hh"
#include "App.hh"
#include "Context.hh"
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
#include "openapi/model/DistSession.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/Tmgi.h"
#include "openapi/model/TunnelAddress.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/PlmnId.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/ProblemCause.hh"

#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"
#include "mb-smf-service-consumer.h"
#include "TimerFunc.hh"

// Header include for this class
#include "UserDataIngSession.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::ObjDistributionData;
using reftools::mbsf::TunnelAddress;
using reftools::mbsf::Ssm;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MbsServiceType;
using reftools::mbsf::IpAddr;
using reftools::mbsf::PlmnId;
using reftools::mbsf::Tmgi;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjectDistrMethInfo;
using reftools::mbsf::ObjAcquisitionMethod;

MBSF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbsf_userdataingsession_api_metadata(
    NMBSF_MBS_UD_INGEST_API_NAME,
    NMBSF_MBS_UD_INGEST_API_VERSION
);

using ActPeriodsType = MBSUserDataIngSession::ActPeriodsType;
using ActPeriodsRepRuleType = MBSUserDataIngSession::ActPeriodsRepRuleType;

static std::optional<std::shared_ptr< Ssm > > ssm(CJson &json, bool as_request);
static bool validate_user_data_ing_session(CJson &json, bool as_request);
static bool resolve_src_dest_addr(const std::string &src_ipv4_addr, const std::string &dest_ipv4_addr, struct addrinfo **ai_src, struct addrinfo **ai_dest);
static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr);
static void process_mbs_distribution_session_info(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::shared_ptr< DistSession > dist_session);
static bool check_for_atleast_one_mbs_dis_sess_info(std::shared_ptr<UserDataIngSession> user_data_ing_session);
static std::string print_mbs_session_error(std::shared_ptr< UserDataIngSession::ContextData > context_data);
static void handle_failed_mbstf_nf_instance_discover(ogs_sbi_xact_t *xact);
static bool validate_state_setting_options(std::shared_ptr<UserDataIngSession> user_data_ing_session, Open5GSSBIStream &stream, Open5GSSBIMessage &message, const NfServer::AppMetadata &app_meta, std::optional<NfServer::InterfaceMetadata> api);
static std::int64_t duration_timer(const std::chrono::system_clock::time_point &tp);

std::recursive_mutex UserDataIngSession::m_mutex;
std::map<ogs_sbi_xact_t *, std::shared_ptr< UserDataIngSession::UserDataIngDistSessId >> UserDataIngSession::m_xactRegistry;
std::map<std::string, std::shared_ptr< UserDataIngSession::UserDataIngDistSessId >> UserDataIngSession::s_distSessionIdRegistry;

UserDataIngSession::UserDataIngSession(CJson &json, bool as_request)
    :m_MBSUserDataIngSession(new MBSUserDataIngSession(json, as_request))
    ,m_hash()
    ,m_UserDataIngSessionId()	
    ,m_sbiObject(new Open5GSSBIObject)
    ,m_rmutex()
    ,m_activePeriods()
    ,m_activePeriodsTimer(nullptr)
    ,m_distSessionState()
    ,m_distributionSessionInfos()	
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


UserDataIngSession::~UserDataIngSession()
{
}

CJson UserDataIngSession::json(bool as_request = false) const
{
    return m_MBSUserDataIngSession->toJSON(as_request);
}


const std::shared_ptr<UserDataIngSession> &UserDataIngSession::find(const std::string &id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::map<std::string, std::shared_ptr<UserDataIngSession> > &UserDataIngSessions = App::self().context()->UserDataIngSessions;
    auto it = UserDataIngSessions.find(id);
    if (it == UserDataIngSessions.end()) {
        throw std::out_of_range("MBS User Data Ingest session not found");
    }
    return it->second;
}

int UserDataIngSession::numberOfDistributionSessions()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
                            static const char *err = "Unable to parse MBSF User Service as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Service", err));
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

			if(!validate_state_setting_options(user_data_ing_session, stream, message, app_meta, api)) return true;
                        
			App::self().context()->addUserDataIngSession(user_data_ing_session);
                        user_data_ing_session->processDistributionSessionInfo(stream_id, request_ctx);

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

                            std::shared_ptr<UserDataIngSession> user_data_ing_sess = UserDataIngSession::find(user_data_ing_session_id);
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
                            std::ostringstream err;
                            err << "User Data Ingest Session [" << user_data_ing_session_id << "] does not exist.";
                            ogs_error("%s", err.str().c_str());

                            static const std::string param("{sessionId}");
                            std::ostringstream reason;
                            reason << "Invalid User Data Ingest Session identifier [" << user_data_ing_session_id << "]";
                            std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                                    app_meta, api, "User Data Ingest Session not found",
                                                                    err.str(), std::nullopt, invalid_params));
                        }
                        return true;
		    } else if (method == OGS_SBI_HTTP_METHOD_PUT) {
	   
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                            app_meta, api, "Method not allowed",
                                                            "The PUT method is not allowed for this path"));
	
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
		                std::shared_ptr<UserDataIngSession> user_data_ing_sess = UserDataIngSession::find(user_data_ing_session_id);
                                user_data_ing_sess->sendMbstfDelRequests();
				//user_data_ing_sess->clearDistributionSessionInfos();
                                std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, std::nullopt, api, app_meta));
                                NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));

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
            UserDataIngSession::UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngSession::UserDataIngDistSessId*>(event.sbiData());
            std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids_ptr = std::make_shared<UserDataIngSession::UserDataIngDistSessId>(*ids);
            
	    try {
                std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids_ptr->first);
                ing_session->nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSession, event.sbiData(), event.sbiData());
                return true;
	    } catch (const std::out_of_range &e) {
                std::ostringstream err;
                err << "MBS User Data Ingest Session [" << ids_ptr->first << "] does not exist.";
                ogs_error("%s", err.str().c_str());
	    }
	    return true;

        }

        case MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION:
        {
            UserDataIngSession::SessionIdContainer *ids = reinterpret_cast<UserDataIngSession::SessionIdContainer*>(event.sbiData());
            try {
	        std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids->second->first);

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
            UserDataIngSession::SessionIdContainer *ids = reinterpret_cast<UserDataIngSession::SessionIdContainer*>(event.sbiData());
            try {
                std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids->second->first);

                ing_session->nmbstfDiscoverAndSend(ids->second, Nmb2Build::buildNmb2DistSessionStatePatch, nullptr, ids);
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

    std::shared_ptr< UserDataIngSession::ContextData > context_data = getContextData(ids);
    if(!context_data->mbstfNFInstanceId.empty()) {
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

   m_activePeriodsTimer = ogs_timer_add(ogs_app()->timer_mgr, UserDataIngSession::changeDistSessionState, (void *)m_UserDataIngSessionId.c_str());
   if (!m_activePeriodsTimer) {
       ogs_error("ogs_timer_add() failed");
   }
   return *this;
}

std::shared_ptr< DistSessionState > UserDataIngSession::getDistSessionState()
{
   std::shared_ptr< DistSessionState > dist_session_state(new DistSessionState());
   DistSessionState dist_sess_state = m_activePeriods->currentState(std::nullopt);
   *dist_session_state = dist_sess_state.getValue();
   {
       std::lock_guard<std::recursive_mutex> lock(m_mutex);
       m_distSessionState = *dist_session_state;
   } 
   return dist_session_state;

}

 const std::string &UserDataIngSession::distSessionState() const 
{
    {
       std::lock_guard<std::recursive_mutex> lock(m_mutex);
       return m_distSessionState;
   } 

};

bool UserDataIngSession::startTimer()
{
    ActivePeriodsBase::TimestampAndActiveFlag transition = m_activePeriods->nextTransition();
    if (transition.first.has_value()) {
	{
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_distSessionState = transition.second;
        } 
	//m_distSessionState = transition.second;    
        std::int64_t dur = duration_timer(transition.first.value());
        ogs_time_t duration = dur;
        ogs_timer_start(m_activePeriodsTimer, ogs_time_from_sec(duration));
	return true;
    }
    return false;

}

void UserDataIngSession::addToDistributionSessionInfos(const std::string &key, const std::shared_ptr< ContextData > context)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_distributionSessionInfos[key] = context;

}

std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::getDistributionSessionInfoData(std::string &key)
{

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_distributionSessionInfos.find(key);
    if (it != m_distributionSessionInfos.end()) {
        return it->second;
    }
    return nullptr;

}

void UserDataIngSession::removeDistributionSessionInfo(std::string &key)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::shared_ptr< UserDataIngSession::ContextData > context_data = getDistributionSessionInfoData(key);
    if(context_data->MBSSession) {
        context_data->MBSSession->deleteSession();
    }
    m_distributionSessionInfos.erase(key);
}


void UserDataIngSession::addToRegistry(ogs_sbi_xact_t* xact, std::shared_ptr< UserDataIngDistSessId > &ids)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_xactRegistry[xact] = ids;

}


void UserDataIngSession::addToRegistry(std::string dist_session_id, std::shared_ptr< UserDataIngDistSessId > &ids)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    s_distSessionIdRegistry[dist_session_id] = ids;

}


void UserDataIngSession::removeFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_xactRegistry.erase(xact);
}

void UserDataIngSession::removeXact(ogs_sbi_xact_t* xact) 
{
    if(!xact) return;
    UserDataIngSession::removeFromRegistry(xact);
    ogs_sbi_xact_remove(xact);
    xact =nullptr;
}

void UserDataIngSession::removeFromRegistry(std::string &dist_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    s_distSessionIdRegistry.erase(dist_session_id);
}


std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > UserDataIngSession::getFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_xactRegistry.find(xact);
    if (it != m_xactRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > UserDataIngSession::getFromRegistry(std::string &dist_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::shared_ptr<UserDataIngSession> user_data_ing_sess = UserDataIngSession::find(user_data_ing_session_id);

        /*
        if(!user_data_ing_sess->checkIfAllMBSTFResponsesReceived()) {
        reset_timer(ids);
        return;
        }
        */

        for (auto it = UserDataIngSession::s_distSessionIdRegistry.begin();
                    it != UserDataIngSession::s_distSessionIdRegistry.end(); )
        {
            if(it->second->first == user_data_ing_session_id) {

                UserDataIngSession::SessionIdContainer* session_id = new SessionIdContainer(it->first, it->second);
                UserDataIngDistSessId *ids = new UserDataIngDistSessId(it->second->first, it->second->second);
                std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids_ptr = std::make_shared<UserDataIngSession::UserDataIngDistSessId>(*ids);
                user_data_ing_sess->nmbstfDiscoverAndSend(ids_ptr, Nmb2Build::buildNmb2DistSessionStatePatch, nullptr, session_id);

            }
            it++;

        }
	if( user_data_ing_sess->m_activePeriods->currentState(std::nullopt) != DistSessionState::NO_VAL) {
            user_data_ing_sess->startTimer();
        }
    }  catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << user_data_ing_session_id << "] does not exist.";
        ogs_error("%s", err.str().c_str());
	
    }

}

void UserDataIngSession::processDistributionSessionInfo( ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request)
{
    bool always_active = true;
    std::shared_ptr< UserDataIngSession::ContextData > ctx_data = nullptr;
    std::shared_ptr< DistSessionState > dist_sess_state = nullptr;

    const ActPeriodsType &act_periods =  m_MBSUserDataIngSession->getActPeriods();
    const ActPeriodsRepRuleType &act_periods_rep_rule = m_MBSUserDataIngSession->getActPeriodsRepRule();
    
    if(act_periods.has_value() && !act_periods->empty()) {
        always_active = false;
        activePeriods(act_periods);
        dist_sess_state = getDistSessionState();
        createTimer();
    }

    if(!act_periods_rep_rule.has_value() || !*act_periods_rep_rule) {
        always_active = false;
    }

    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = m_MBSUserDataIngSession->getMbsDisSessInfos();
    mb_smf_sc_mbs_session_t *session = NULL;

    for(const auto &[key, sess_info]: dist_sess_infos) {
        if(sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = sess_info.value();
            if(info) {
                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if(mbs_session_id.has_value()) {
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
                        ogs_sbi_xact_t *xact = nullptr;

                        ssm_data.reset(new Ssm(*ssm_val));
                        if (dest_ipv4_addr.has_value() || dest_ipv6_addr.has_value()) {
                            ctx_data.reset(new UserDataIngSession::ContextData{std::string(m_UserDataIngSessionId), std::string(key), info, ssm_data, request, stream_id, nullptr});
                            addToDistributionSessionInfos(std::string(key), ctx_data);
                            xact = nmbstfDiscoverOnly(ctx_data);

                        } else {
                            ogs_error("Unable to resolve SSM addresses");
                            continue;
                        }

                    }

                }
		if(always_active) {
                    std::shared_ptr<AlwaysActive> always_active(new AlwaysActive());

                    std::shared_ptr< DistSessionState > dist_session_state(new DistSessionState());
                    always_active->currentState(std::nullopt);
                    info->setMbsDistSessState(dist_session_state);

                    ActivePeriodsBase::TimestampAndActiveFlag transition = always_active->nextTransition();

                } else {
                    if(dist_sess_state) info->setMbsDistSessState(dist_sess_state);
                }
            }
        }
    }
    startTimer();
}

bool UserDataIngSession::processDistSession(std::shared_ptr< DistSession > dist_session)
{
    if(!dist_session) return false;

    std::shared_ptr< UserDataIngSession::ContextData > context_data = nullptr;
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> ids = nullptr;

    {
        std::lock_guard<std::recursive_mutex> lock(UserDataIngSession::m_mutex);
        auto it = UserDataIngSession::s_distSessionIdRegistry.find(dist_session->getDistSessionId());
        if (it != UserDataIngSession:: s_distSessionIdRegistry.end()) {
            ids = it->second;
        }
    }

    context_data = UserDataIngSession::getContextData(ids);

    process_mbs_distribution_session_info(context_data, dist_session);

    context_data->receivedMBSTFResponse = true;

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
        if(ing_sess->checkIfAllMBSTFResponsesReceived()) {
            ing_sess->sendNmbsfMbsUserDataIngestResponse(ids);
        }
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }

    return true;
}

bool UserDataIngSession::checkIfAllMBSTFResponsesReceived()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->receivedMBSTFResponse) {
            return false;
        }
    }
    return true;

}

bool UserDataIngSession::checkIfAllMBSTFPatchResponsesReceived()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->receivedMBSTFPatchResponse) {
            return false;
        }
    }
    return true;

}


bool UserDataIngSession::resetReceivedMBSTFResponseFlags()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        dist_sess_info.second->receivedMBSTFResponse = false; 
    }
    return true;

}


bool UserDataIngSession::sendNmbsfMbsUserDataIngestResponse(std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids)
{

    std::shared_ptr< UserDataIngSession::ContextData > context_data = UserDataIngSession::getContextData(ids);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
        const std::shared_ptr<MBSUserDataIngSession> &user_data_ing_session = ing_sess->getMBSUserIngSession();
    
        std::shared_ptr<Open5GSSBIRequest> request = context_data->request;
        Open5GSSBIMessage message;
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
    if(!nf_instance) {
        handle_failed_mbstf_nf_instance_discover(xact);
        return false;
    }   

    Open5GSSBIClient mbstf_client(reinterpret_cast<ogs_sbi_client_t*>(NF_INSTANCE_CLIENT(nf_instance)));
    std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session;
    std::string src_ipv4_addr;
    std::string src_ipv6_addr;

    ogs_sockaddr_t *client_ipv4_addr =  mbstf_client.ogsSBIClientIPv4Addr();
    ogs_sockaddr_t *client_ipv6_addr =  mbstf_client.ogsSBIClientIPv6Addr();

    if(client_ipv4_addr) {
	char *ipv4_addr = mbstf_client.ogsIpStrdup(client_ipv4_addr);    
        src_ipv4_addr = std::string(ipv4_addr);
	ogs_free(ipv4_addr);
    }

    if(client_ipv6_addr) {
	 char *ipv6_addr = mbstf_client.ogsIpStrdup(client_ipv6_addr);
         src_ipv6_addr = std::string(ipv6_addr);
	 ogs_free(ipv6_addr);
    }


    std::shared_ptr< UserDataIngSession::ContextData > context_data = nullptr;
    std::shared_ptr< UserDataIngDistSessId > ids = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_xactRegistry.find(xact);
        if (it != m_xactRegistry.end()) {
            ids = it->second;
        }
    }

    if(!ids) {
        return false;
    }

    std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids->first);
    if(nf_instance->t_validity)  ogs_timer_stop(nf_instance->t_validity);
    ing_session->setNFInstance(xact->service_type, nf_instance);


    context_data = getContextData(ids);
    if(!context_data) {
        ogs_error("Unable to get context data from registry");
        return false;
    }

    if(context_data->mbstfNFInstanceId.empty()) context_data->mbstfNFInstanceId = std::string(nf_instance->id);

    std::shared_ptr<Ssm> ssm_ptr = context_data->ssm;
    if(!ssm_ptr) ogs_error("Unable to get SSM from Context Data");

    std::shared_ptr< IpAddr > dest_ip_addr = ssm_ptr->getDestIpAddr();
    std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
    std::optional<std::shared_ptr< std::string > > dest_ipv6_addr = dest_ip_addr->getIpv6Addr();

    if(!client_ipv4_addr && !client_ipv6_addr) {
        ogs_info("Source SSM cannot be resolved");
        mb_smf_mbs_session.reset(new MBSMFMBSSession(mb_smf_sc_mbs_session_new()));
        mb_smf_mbs_session->setTunnelRequest(true);

    } else if (client_ipv4_addr && dest_ipv4_addr.has_value()) {
        struct addrinfo *ai_src = NULL, *ai_dest = NULL;
        void *src_addr = NULL, *dest_addr = NULL;

        if(resolve_src_dest_addr(src_ipv4_addr, dest_ipv4_addr.value(), &ai_src, &ai_dest))
        {
            if(get_src_dest_of_same_addr_family(AF_INET, ai_src, ai_dest, &src_addr, &dest_addr))
            {
                mb_smf_mbs_session.reset(new MBSMFMBSSession(
                mb_smf_sc_mbs_session_new_ipv4((const struct in_addr*)src_addr,
                                                     (const struct in_addr*)dest_addr)));

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv4 address family");
               if(ai_src) {
                   freeaddrinfo(ai_src);
                   ai_src = NULL;
               }

               if(ai_dest) {
                   freeaddrinfo(ai_dest);
                   ai_dest = NULL;
                }
                return false;
            }
        }
        if(ai_src) {
            freeaddrinfo(ai_src);
            ai_src = NULL;
        }

        if(ai_dest) {
            freeaddrinfo(ai_dest);
            ai_dest = NULL;
        }

    } else if (client_ipv6_addr && dest_ipv6_addr.has_value()) {
       struct addrinfo *ai_src = NULL, *ai_dest = NULL;
       void *src_addr = NULL, *dest_addr = NULL;
       std::shared_ptr< std::string >  dest_ipv6 = dest_ipv6_addr.value();

       if(resolve_src_dest_addr(src_ipv6_addr, *dest_ipv6, &ai_src, &ai_dest))
       {
           if(get_src_dest_of_same_addr_family(AF_INET6, ai_src, ai_dest, &src_addr, &dest_addr))
           {
               mb_smf_mbs_session.reset(new MBSMFMBSSession(
               mb_smf_sc_mbs_session_new_ipv6((const struct in6_addr*)src_addr,
                               (const struct in6_addr*)dest_addr)));

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv6 address family");
               if(ai_src) freeaddrinfo(ai_src);
               if(ai_dest) freeaddrinfo(ai_dest);
               return false;
           }
       }
       if(ai_src) freeaddrinfo(ai_src);
       if(ai_dest) freeaddrinfo(ai_dest);

    } else {
       ogs_error("Unable to resolve SSM addresses");
       return false;
    }

    if(mb_smf_mbs_session->ssm()) {

        mb_smf_mbs_session->setTunnelRequest(true);
        mb_smf_mbs_session->setTmgiRequest(true);

        mb_smf_mbs_session->setServiceType(MBS_SERVICE_TYPE_MULTICAST);
        if(!context_data->MBSSession) context_data->MBSSession = mb_smf_mbs_session;
	UserDataIngDistSessId *ids = new UserDataIngDistSessId(context_data->ingSessionId, context_data->distSessionInfoKey);
        mb_smf_mbs_session->setCreatedCallback(reinterpret_cast<void*>(ids) /*(context_data.get()*/);

        context_data->MBSSession = mb_smf_mbs_session;

        mb_smf_mbs_session->pushChanges();
    } else {
        ogs_info("MB-SMF SSM is not present");
    }

    UserDataIngSession::removeFromRegistry(xact);

    return true;

}

void UserDataIngSession::deleteMBSTFSession(ogs_sbi_xact_t *xact)
{
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> ids = nullptr;

    {
        std::lock_guard<std::recursive_mutex> lock(UserDataIngSession::m_mutex);
        auto it = UserDataIngSession::m_xactRegistry.find(xact);
        if (it != UserDataIngSession::m_xactRegistry.end()) {
            ids = it->second;
        }
    }

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
        ing_sess->sendMbstfDelRequests();
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
}

void UserDataIngSession::handlePatchUpdateResponse(ogs_sbi_xact_t *xact)
{
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> ids = nullptr;

     {
        std::lock_guard<std::recursive_mutex> lock(UserDataIngSession::m_mutex);
        auto it = UserDataIngSession::m_xactRegistry.find(xact);
        if (it != UserDataIngSession::m_xactRegistry.end()) {
            ids = it->second;
        }
    }

    try {
        std::shared_ptr< UserDataIngSession::ContextData > context_data = UserDataIngSession::getContextData(ids);
        context_data->receivedMBSTFPatchResponse = true;
        context_data->patchUpdateSucceded = true;
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }

}


void UserDataIngSession::rollbackMBSTFDistSessionState(ogs_sbi_xact_t *xact)
{
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> ids = nullptr;


    {
        std::lock_guard<std::recursive_mutex> lock(UserDataIngSession::m_mutex);
        auto it = UserDataIngSession::m_xactRegistry.find(xact);
        if (it != UserDataIngSession::m_xactRegistry.end()) {
            ids = it->second;
        }
    }

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);

        std::shared_ptr< UserDataIngSession::ContextData > context_data = UserDataIngSession::getContextData(ids);

        context_data->receivedMBSTFPatchResponse = true;
        context_data->patchUpdateSucceded = false;
        if(ing_sess->checkIfAllMBSTFPatchResponsesReceived()) {
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
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
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
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }

}

void UserDataIngSession::clearDistributionSessionInfos()
{
    for (auto &dist_sess_info : m_distributionSessionInfos)
    {
        std::shared_ptr<UserDataIngSession::ContextData> context_data = dist_sess_info.second;
        if(context_data->MBSSession) {
	    context_data->MBSSession->deleteSession();
	}

    }

    m_distributionSessionInfos.clear();
}

bool UserDataIngSession::checkIfAllMBSSessionCreated()
{
    bool all_mbs_sessions_created = true;

    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus != MBSSessionState::CREATED) {
            all_mbs_sessions_created = false;
            break;
        }
    }
    if(all_mbs_sessions_created) {
        sendMbstfRequests();

    }
    return all_mbs_sessions_created;

}

void UserDataIngSession::sendMbstfRequests()
{
    for (auto &dist_sess_info : m_distributionSessionInfos)
    {
        std::shared_ptr<UserDataIngSession::ContextData> context_data = dist_sess_info.second;

        UserDataIngDistSessId *ids = new UserDataIngDistSessId(context_data->ingSessionId, context_data->distSessionInfoKey);

        sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_REQ_BUILD, reinterpret_cast<void *>(ids));

    }
}

bool UserDataIngSession::checkIfAllMBSTFDistSessionDeleted()
{
    bool all_mbstf_dist_session_deleted = true;

    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->MBSTFDistSessionDeleted) {
            all_mbstf_dist_session_deleted = false;
            break;
        }
    }

    if(all_mbstf_dist_session_deleted) {
        for (auto it = UserDataIngSession::s_distSessionIdRegistry.begin();
                    it != UserDataIngSession::s_distSessionIdRegistry.end(); )
        {
            if(it->second->first == m_UserDataIngSessionId) {
	        it = UserDataIngSession::s_distSessionIdRegistry.erase(it);
	    }
	}
    
        App::self().context()->deleteUserDataIngSession(m_UserDataIngSessionId);

    }
    
    return all_mbstf_dist_session_deleted;
}

void UserDataIngSession::sendMbstfDelRequests()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto it = UserDataIngSession::s_distSessionIdRegistry.begin();
                    it != UserDataIngSession::s_distSessionIdRegistry.end(); )
    {
        if(it->second->first == m_UserDataIngSessionId) {
            	
            UserDataIngSession::SessionIdContainer* session_id = new SessionIdContainer(it->first, it->second);
	    it++;

            sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION, session_id);
	}

    }
}

void UserDataIngSession::sendMbstfPatchRollbackRequests()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto it = UserDataIngSession::s_distSessionIdRegistry.begin();
                    it != UserDataIngSession::s_distSessionIdRegistry.end(); )
    {
        if(it->second->first == m_UserDataIngSessionId) {

            UserDataIngSession::SessionIdContainer* session_id = new SessionIdContainer(it->first, it->second);
            it++;

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

    if (event->id == MBSF_LOCAL_SEND_MBSTF_REQ_BUILD)
    {
        return "MBSF_LOCAL_SEND_MBSTF_REQ_BUILD";
    } else if (event->id == MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK)
    {
        return "MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK";
    } else {

        return ogs_event_get_name(event);
    }

    return "Unknown MBSF LOCAL Event";
}

void UserDataIngSession::setMBSSessionFlag(void *data)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
        std::shared_ptr< UserDataIngSession::ContextData > context_data = ing_sess->getDistributionSessionInfoData(ids->second);
        context_data->MBSSessionStatus = MBSSessionState::CREATED;
	if(ing_sess->checkIfAllMBSSessionResponsesReceived()) {
	    bool rv = ing_sess->checkIfAllMBSSessionCreated();
	    if(!rv) {
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

void UserDataIngSession::setMBSSessionFailureFlag(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    try {
        std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
        std::shared_ptr< UserDataIngSession::ContextData > context_data = ing_sess->getDistributionSessionInfoData(ids->second);
        context_data->MBSSessionStatus = MBSSessionState::FAILED;
        //context_data->hasMBSSession = true;
        if(ing_sess->checkIfAllMBSSessionResponsesReceived()) {
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
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (dist_sess_info.second->MBSSessionStatus == MBSSessionState::FAILED) {
            UserDataIngDistSessId *ids = new UserDataIngDistSessId(dist_sess_info.second->ingSessionId, dist_sess_info.second->distSessionInfoKey);
            populateAndSendError(reinterpret_cast<void*>(ids), dist_sess_info.second->mbsmfProblemCause, dist_sess_info.second->mbsmfProblemDetailJson);
        }
    }
}


bool UserDataIngSession::checkIfAllMBSSessionResponsesReceived()
{
    bool all_mbs_sessions_responses_received = true;

    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!((dist_sess_info.second->MBSSessionStatus == MBSSessionState::CREATED) || (dist_sess_info.second->MBSSessionStatus == MBSSessionState::FAILED))) {
            all_mbs_sessions_responses_received = false;
        }
    }
    return all_mbs_sessions_responses_received;

}

void UserDataIngSession::setMBSTFDistSessionDeletedFlag(std::string &dist_session_id)
{

    std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > ids = getFromRegistry(dist_session_id);
    
    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    std::shared_ptr< UserDataIngSession::ContextData > context_data = getContextData(ids);
    context_data->MBSTFDistSessionDeleted = true;
    ing_sess->checkIfAllMBSTFDistSessionDeleted();

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

    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);
    std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids_ptr = std::make_shared<UserDataIngSession::UserDataIngDistSessId>(*ids);

    std::shared_ptr< UserDataIngSession::ContextData > context_data = getContextData(ids_ptr);

    std::optional<NfServer::InterfaceMetadata> api(std::nullopt);
    const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();

    std::shared_ptr<Open5GSSBIRequest> request = context_data->request;
    Open5GSSBIStream stream(context_data->streamId);
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

    if(cause.has_value()) {
        ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, cause.value(), error.c_str()));
	
    } else {

	ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, ProblemCause::INBOUND_SERVER_ERROR, error.c_str()));

    }
}


bool UserDataIngSession::tmgi(mb_smf_sc_tmgi_t *tmgi, void *data)
{
    bool tmgi_set = false;
    char buf[OGS_PLMNIDSTRLEN];

    const char *tmgi_repr = mb_smf_sc_tmgi_repr(tmgi);
    char *plmn_id = ogs_plmn_id_to_string(&tmgi->plmn, buf);
    char *mcc = ogs_plmn_id_mcc_string(&tmgi->plmn);
    char *mnc = ogs_plmn_id_mnc_string(&tmgi->plmn);

    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);
    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    std::shared_ptr< UserDataIngSession::ContextData > context_data = ing_sess->getDistributionSessionInfoData(ids->second);
    if(context_data && context_data->info) {
	if(tmgi) context_data->tmgi = tmgi;
        std::shared_ptr<Tmgi> mgi = nullptr;
        std::shared_ptr<PlmnId> plmn_id = nullptr;

        plmn_id.reset(new PlmnId());
        plmn_id->setMcc(std::string(mcc));
        plmn_id->setMnc(std::string(mnc));

        mgi.reset(new Tmgi());
        mgi->setMbsServiceId(std::string(tmgi->mbs_service_id));
        mgi->setPlmnId(plmn_id);

        std::optional<std::shared_ptr< MbsSessionId > > mbs_sess_id = context_data->info->getMbsSessionId();
        if(mbs_sess_id.has_value()) {
            std::shared_ptr< MbsSessionId > sess_id = mbs_sess_id.value();
            sess_id->setTmgi(mgi);
            tmgi_set = true;
        }
    }

    ogs_info(" TMGI [%s], SERVICE ID [%s], PLMN [%s], MCC [%s], MNC [%s]", tmgi_repr, tmgi->mbs_service_id, ogs_plmn_id_to_string(&tmgi->plmn, buf), ogs_plmn_id_mcc_string(&tmgi->plmn), ogs_plmn_id_mnc_string(&tmgi->plmn));
    ogs_free(mcc);
    ogs_free(mnc);

    return tmgi_set;

}


std::shared_ptr< ObjDistributionOperatingMode > UserDataIngSession::getOperatingMode(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getOperatingMode();
    }
    return nullptr;
}

std::shared_ptr< ObjAcquisitionMethod > UserDataIngSession::getAcquisitionMethod(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqMethod();
    }
    return nullptr;
}

std::optional<std::string> UserDataIngSession::getObjectIngestUrl(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjIngUri();
    }
    return std::nullopt;
}

std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > UserDataIngSession::getObjectAcquisitionIds(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqIds();
    }
    return {};
}

std::optional<std::string> UserDataIngSession::getObjectDistributionUrl(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjDistrUri();
    }
    return std::nullopt;
}

std::string UserDataIngSession::maxContBitRate(std::shared_ptr<MBSDistributionSessionInfo> &info)
{
    return info->getMaxContBitRate();
}

static void process_mbs_distribution_session_info(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::shared_ptr< DistSession > dist_session)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > >  obj_distribution_method_info = context_data->info->getObjDistrInfo();
    if(obj_distribution_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > obj_dist_method_info = obj_distribution_method_info.value();
        if(obj_dist_method_info->getOperatingMode()->getString() == "PUSH") {
            std::optional<std::shared_ptr< ObjDistributionData > > obj_distribution_data = dist_session->getObjDistributionData();
            if(obj_distribution_data.has_value()) {
                std::shared_ptr< ObjDistributionData > obj_dist_data = obj_distribution_data.value();
                std::shared_ptr< ObjAcquisitionMethod> obj_acquisition_method = obj_dist_data->getObjAcquisitionMethod();
                      if(obj_acquisition_method->getString() == "PUSH") {
                    obj_dist_method_info->setObjIngUri(obj_dist_data->getObjIngestBaseUrl());
                }

            }

        }

    }
}

static bool check_for_atleast_one_mbs_dis_sess_info(std::shared_ptr<UserDataIngSession> user_data_ing_session)
{
    if(!user_data_ing_session) return false;
    const std::shared_ptr<MBSUserDataIngSession> ing_session = user_data_ing_session->getMBSUserIngSession(); 
    const auto &dist_sess_infos = ing_session->getMbsDisSessInfos();

    if (dist_sess_infos.empty()) return false;

    for (const auto& [key, sess_info] : dist_sess_infos)
    {
        if (sess_info && *sess_info)
        {
            return true;
        }
    }

    return false;
}


static std::optional<std::shared_ptr< Ssm > > ssm(CJson &json, bool as_request)
{
    MBSUserDataIngSession user_data_ing_session(json, as_request);
    std::string mbs_user_service_id(user_data_ing_session.getMbsUserServId());
    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = user_data_ing_session.getMbsDisSessInfos();
    for(const auto &[key, sess_info]: dist_sess_infos) {
        if(sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = *sess_info;
            if(info) {
                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if(mbs_session_id.has_value()) {
                    std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                    return mbs_sess_id->getSsm();
                }
            }
        }
    }
    return std::nullopt;

}

static bool validate_user_data_ing_session(CJson &json, bool as_request)
{
    MBSUserDataIngSession user_data_ing_session(json, as_request);
    std::string mbs_user_service_id(user_data_ing_session.getMbsUserServId());
    const std::shared_ptr<UserService> user_service = UserService::find(mbs_user_service_id);
    std::string user_service_type(user_service->getMBSUserServiceType());
    const MBSUserDataIngSession::MbsDisSessInfosType &distSessInfos = user_data_ing_session.getMbsDisSessInfos();
    for(const auto &[key, sess_info]: distSessInfos) {
        if(sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = *sess_info;
            if(info) {
                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if(mbs_session_id.has_value()) {
                    std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                    std::optional<std::shared_ptr< Ssm > > ssm = mbs_sess_id->getSsm();
                    if (ssm.has_value()) {
                        std::shared_ptr< Ssm > ssm_val = ssm.value();
                        std::shared_ptr< IpAddr > dest_ip_addr = ssm_val->getDestIpAddr();
                        std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();

                        if((user_service_type == "BROADCAST")) {
                            throw std::logic_error("Invalid Source-Specific Multicast (SSM) User Plane address for Broadcast Service Type");
                        }

                    }
                }
            }
        }
    }
    return true;
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
    if(!ogs_stream) return;
    Open5GSSBIStream stream(xact->assoc_stream_id);

    ogs_assert(true == Open5GSSBIServer::sendError(stream, OGS_SBI_HTTP_STATUS_INTERNAL_SERVER_ERROR, std::nullopt,
                                 "Unable to Discover MBSTF", "MBSTF discovery failed" , "No MBSTF found in the network"));

}

static std::int64_t duration_timer(const std::chrono::system_clock::time_point  &tp) {
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(tp - now).count();
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
    for (auto &[dist_sess_id, dist_sess_info] : mbs_user_data_ing_session->getMbsDisSessInfos()) {
	if(dist_sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = dist_sess_info.value();
            if(info) {
		std::optional<std::shared_ptr< DistSessionState > > dist_session_state = info->getMbsDistSessState();
		if(dist_session_state.has_value()){
	            std::shared_ptr< DistSessionState > dist_sess_state = dist_session_state.value();
		    if(dist_sess_state->getString() == "DEACTIVATING") {
		        invalid_params[std::format("mbsDisSessInfos.{}.mbsDistSessState = {}", dist_sess_id, dist_sess_state->getString())] = "mbsDistSessState cannot be DEACTIVATING";
		    }
		}
		if(dist_session_state.has_value() && mbs_user_data_ing_session->getActPeriods()) {
                   invalid_params["actPeriods"] = "actPeriods cannot be present if any mbsDistSessState or actPeriodRepRule are present";
                   invalid_params[std::format("mbsDisSessInfos.{}.mbsDistSessState", dist_sess_id)] = "mbsDistSessState cannot be present if actPeriods or actPeriodRepRule are present";
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

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

