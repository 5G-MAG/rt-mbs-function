/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS User Service class
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
#include <string>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "DistributionSession.hh"
#include "hash.hh"
#include "MBSFNetworkFunction.hh"
#include "NfServer.hh"
#include "Open5GSEvent.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSNetworkFunction.hh"
#include "openapi/model/MBSUserService.h"
#include "openapi/model/TunnelAddress.h"

#include "openapi/api/IndividualMBSUserServiceDocumentApi-info.h"
#include "TimerFunc.hh"

// Header include for this class
#include "UserService.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::MBSUserService;
using reftools::mbsf::CreateReqData;
using reftools::mbsf::TunnelAddress;

MBSF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbsf_userservice_api_metadata(
    NMBSF_MBS_US_API_NAME,
    NMBSF_MBS_US_API_VERSION
);


UserService::UserService(CJson &json, bool as_request)
    :m_MBSUserService(std::make_shared<MBSUserService>(json, as_request))
{
    ogs_uuid_t uuid;

    char id[OGS_UUID_FORMATTED_LENGTH + 1];

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

    std::string json_str(json.serialise());
    m_hash = calculate_hash(std::vector<std::string::value_type>(json_str.begin(), json_str.end()));

    m_UserServiceId = std::string(id);


    //App::self().context()->addUserService(std::string(m_UserServiceId), std::shared_ptr<UserService> UserService)
}

UserService::~UserService()
{
}

CJson UserService::json(bool as_request = false) const
{
    return m_MBSUserService->toJSON(as_request);
}

void UserService::update(CJson &json, bool as_request)
{
    m_MBSUserService.reset(new MBSUserService(json, as_request));
    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

}


const std::shared_ptr<UserService> &UserService::find(const std::string &id)
{
    const std::map<std::string, std::shared_ptr<UserService> > &UserServices = App::self().context()->UserServices;
    auto it = UserServices.find(id);
    if (it == UserServices.end()) {
        throw std::out_of_range("MBS User Service not found");
    }
    return it->second;
}

bool UserService::processEvent(Open5GSEvent &event)
{

    const NfServer::InterfaceMetadata &nmbsf_mbs_userservice_api = g_nmbsf_userservice_api_metadata;
    const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();

    switch (event.id()) {
    case OGS_EVENT_SBI_SERVER:
        {
            Open5GSSBIRequest request(event.sbiRequest());
            ogs_assert(request);
            Open5GSSBIMessage message;
            Open5GSSBIStream stream(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData()));
            ogs_assert(stream);
            Open5GSSBIServer server(stream.server());
            ogs_assert(server);
            std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("Failed to parse request headers");
                break;
            }

            std::string service_name(message.serviceName());
            std::string resource0(message.resourceComponent(0));
            ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", service_name.c_str(), resource0.c_str());
            if (service_name == "nmbsf-mbs-us") {
                api.emplace(nmbsf_mbs_userservice_api);
            } else {
                return false;
            }

            if (api.value() == nmbsf_mbs_userservice_api) {
                /******** nmbsf-mbs-us ********/
                std::string api_version(message.apiVersion());
                if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Unsupported API version [%s]", api_version.c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta,
                                                           api, "Unsupported API version"));
                    return true;
                }

                if (resource0 == "mbs-user-services") {
                    std::string method(message.method());
                    const char *ptr_resource1 = message.resourceComponent(1);
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
                        ogs_debug("POST response: status = %i", message.resStatus());
			std::shared_ptr<UserService> user_service;
                        ogs_debug("Request body: %s", request.content());
                        //ogs_debug("Request " OGS_SBI_CONTENT_TYPE ": %s", request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()).c_str());
                        if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                            return true;
                        }

                        CJson mbs_user_service(CJson::Null);
                        try {
                            mbs_user_service = CJson::parse(request.content());
                        } catch (std::exception &ex) {
                            static const char *err = "Unable to parse MBSF User Service as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Service", err));
                            return true;
                        }

                        {
                            std::string txt(mbs_user_service.serialise());
                            ogs_debug("Request Parsed JSON: %s", txt.c_str());
                        }

                        try {
                            user_service.reset(new UserService(mbs_user_service, true));
                        } catch (std::exception &err) {
                            ogs_error("Error while populating MBSF Session: %s", err.what());
                            char *error = ogs_msprintf("Bad request [%s]", err.what());
                            ogs_error("%s", error);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message,
                                                                            app_meta, api, "Bad Request", error));
                            ogs_free(error);
                            return true;
                        }

                        App::self().context()->addUserService(user_service);

                        CJson mbs_user_service_json(user_service->json(false));
                        std::string body(mbs_user_service_json.serialise());
                        ogs_debug("Response Parsed JSON: %s", body.c_str());
                        std::ostringstream location;
                        location << request.uri() << "/" << user_service->userServiceId();
                        std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                                                                                    body.empty()?nullptr:"application/json",
                                                                                    user_service->generated(),
                                                                                    user_service->hash().c_str(),
                                                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                                                    std::nullopt/*nullptr*/, api, app_meta));
                        ogs_assert(response);
                        NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_CREATED);
                        ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
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
                        std::string user_service_id(ptr_resource1);
                        try {
                            int response_code = 200;

                            std::shared_ptr<UserService> user_serv = UserService::find(user_service_id);
                            CJson user_service_json(user_serv->json(false));
                            std::string body(user_service_json.serialise());
                            ogs_debug("Parsed JSON: %s", body.c_str());
                            std::ostringstream location;
                            location << request.uri() << "/" << user_serv->userServiceId();
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::string(request.uri())/*location.str()*/,
                                                    body.empty()?nullptr:"application/json",
                                                    user_serv->generated(),
                                                    user_serv->hash().c_str(),
                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                    std::nullopt/*nullptr*/, api, app_meta));
                            ogs_assert(response);
                            NfServer::populateResponse(response, body, response_code);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                        } catch (const std::out_of_range &e) {
                            std::ostringstream err;
                            err << "User Service [" << user_service_id << "] does not exist.";
                            ogs_error("%s", err.str().c_str());

                            static const std::string param("{UserServiceId}");
                            std::ostringstream reason;
                            reason << "Invalid MBS UserService identifier [" << user_service_id << "]";
                            std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                                    app_meta, api, "MBSF User Service not found",
                                                                    err.str(), std::nullopt, invalid_params));
                        }
                        return true;
		    
		    } else if (method == OGS_SBI_HTTP_METHOD_PUT) {
			const char *ptr_resource2 = message.resourceComponent(2);
			if (!ptr_resource1 && !ptr_resource2) {
                            std::ostringstream err;
                            err << "Invalid resource [" << message.uri() << "]";
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Request", err.str()));
                            return true;
                        }
			 if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                            return true;
                        }

                        CJson mbs_user_service(CJson::Null);
                        try {
                            mbs_user_service = CJson::parse(request.content());
                        } catch (std::exception &ex) {
                            static const char *err = "Unable to parse MBSF User Service as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Service", err));
                            return true;
                        }

                        {
                            std::string txt(mbs_user_service.serialise());
                            ogs_debug("Request Parsed JSON: %s", txt.c_str());
                        }

                        std::string user_service_id(ptr_resource1);
			try {
                            int response_code = 200;

                            std::shared_ptr<UserService> user_service = UserService::find(user_service_id);
			    user_service->update(mbs_user_service, true);
                            CJson user_service_json(user_service->json(false));
                            std::string body(user_service_json.serialise());
                            ogs_debug("Parsed JSON: %s", body.c_str());
                            std::ostringstream location;
                            location << request.uri() << "/" << user_service->userServiceId();
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::string(request.uri())/*location.str()*/,
                                                    body.empty()?nullptr:"application/json",
                                                    std::chrono::time_point_cast<std::chrono::seconds>(user_service->generated()),
                                                    user_service->hash().c_str(),
                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                    std::nullopt/*nullptr*/, api, app_meta));
                            ogs_assert(response);
                            NfServer::populateResponse(response, body, response_code);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                        } catch (const std::out_of_range &e) {
                            std::ostringstream err;
                            err << "User Service [" << user_service_id << "] does not exist.";
                            ogs_error("%s", err.str().c_str());

                            static const std::string param("{mbsUserServId}");
                            std::ostringstream reason;
                            reason << "Invalid MBS User Service identifier [" << user_service_id << "]";
                            std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                                    app_meta, api, "MBS User Service not found",
                                                                    err.str(), std::nullopt, invalid_params));
                        }

                        return true; 
		 	
                    } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
                        if (message.resourceComponent(1) && !message.resourceComponent(2)) {
                            std::string user_service_id(message.resourceComponent(1));
                            try {
                                App::self().context()->deleteUserService(user_service_id);
                                std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, std::nullopt, api, app_meta));
                                NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));

                            } catch (const std::out_of_range &e) {
                                std::ostringstream err;
                                err << "MBS Session [" << user_service_id << "] does not exist.";
                                ogs_error("%s", err.str().c_str());

                                static const std::string param("{userServiceId}");
                                std::ostringstream reason;
                                reason << "Invalid MBS User Service identifier [" << user_service_id << "]";
                                std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                        app_meta, api, "MBS User Service not found", err.str(),
                                                        std::nullopt, invalid_params));
                            }
                            return true;
                        }
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
                    err << "Unknown object type \"" << message.resourceComponent(0) << "\" in MBSF Distribution Session";
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
        default:
            break;
    }
    return false;
}

void UserService::addDistributionSession(const std::shared_ptr<DistributionSession> &session)
{
    std::shared_ptr<DistributionSession> map_session(session);
    m_distributionSessions.insert(std::make_pair<std::string, std::shared_ptr<DistributionSession> >(std::string(map_session->distributionSessionId()), std::move(map_session)));
}


void UserService::deleteDistributionSession(const std::string &distributionSessionid)
{
    auto it = m_distributionSessions.find(distributionSessionid);
    if (it != m_distributionSessions.end()) {
        m_distributionSessions.erase(it);
    } else {
        throw std::out_of_range("MBSF Distribution session not found");
    }
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
