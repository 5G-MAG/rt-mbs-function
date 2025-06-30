/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSF Open5GS Event Handler
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
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

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <stdexcept>
#include <string>

#include "common.hh"
#include "App.hh"
#include "UserService.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIStream.hh"

#include "MBSFEventHandler.hh"

MBSF_NAMESPACE_START

void MBSFEventHandler::dispatch(Open5GSFSM &fsm, Open5GSEvent &event)
{
    // Handle Open5GS FSM events here
    ogs_debug("MBSF Event: %s", ogs_event_get_name(event.ogsEvent()));

    if (UserService::processEvent(event)) return;


    switch (event.id()) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("[%s] MBSF Running", ogs_sbi_self()->nf_instance->id);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case OGS_EVENT_SBI_SERVER:
        {
            Open5GSSBIRequest request(event.sbiRequest());
            Open5GSSBIStream stream(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData()));

            Open5GSSBIMessage message;

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("ogs_sbi_parse_header() failed");
                ogs_assert(true == Open5GSSBIServer::sendError(
                                stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                message, "cannot parse HTTP message", nullptr));
                break;
            }

            std::string service_name(message.serviceName());
            if (service_name == OGS_SBI_SERVICE_NAME_NNRF_NFM) {
                std::string api_version(message.apiVersion());
                if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Not supported version [%s]", api_version.c_str());
                    ogs_assert(true == Open5GSSBIServer::sendError(
                                stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                message, "Not supported version", nullptr));
                    break;
                }
                std::string resource(message.resourceComponent(0));
                if (resource == OGS_SBI_RESOURCE_NAME_NF_STATUS_NOTIFY) {
                    std::string method(message.method());
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
                        ogs_nnrf_nfm_handle_nf_status_notify(stream.ogsSBIStream(), message.ogsSBIMessage());
                    } else {
                        ogs_error("Invalid HTTP method [%s]", method.c_str());
                        ogs_assert(true == Open5GSSBIServer::sendError(stream,
                                        OGS_SBI_HTTP_STATUS_FORBIDDEN, message,
                                        "Invalid HTTP method", method.c_str()));
                    }
                } else {
                    ogs_error("Invalid resource name [%s]", resource.c_str());
                    ogs_assert(true == Open5GSSBIServer::sendError(stream,
                                    OGS_SBI_HTTP_STATUS_BAD_REQUEST, message,
                                    "Invalid resource name",
                                    resource.c_str()));
                }
            } else {
                ogs_error("Invalid API name [%s]", service_name.c_str());
                ogs_assert(true == Open5GSSBIServer::sendError(stream,
                                        OGS_SBI_HTTP_STATUS_BAD_REQUEST, message,
                                        "Invalid API name.", message.method()));
            }
        }
        break;

    case OGS_EVENT_SBI_CLIENT:
        {
            ogs_assert(event.ogsEvent());

            Open5GSSBIResponse response(event.sbiResponse(true));
            Open5GSSBIMessage message;
            try {
                message.parseHeader(response);
            } catch (std::exception &ex) {
                ogs_error("ogs_sbi_parse_header() failed decoding client response");
                break;
            }
            message.resStatus(response.status());

            std::string service_name(message.serviceName());
            if (service_name == OGS_SBI_SERVICE_NAME_NNRF_NFM) {
                std::string resource(message.resourceComponent(0));
                if (resource == OGS_SBI_RESOURCE_NAME_NF_INSTANCES) {
                    cJSON *nf_profile;
                    OpenAPI_nf_profile_t *nfprofile;
                    ogs_sbi_nf_instance_t *nf_instance = reinterpret_cast<ogs_sbi_nf_instance_t*>(event.sbiData());

                    ogs_assert(nf_instance);

                    if (response.contentLength() && response.content()){
                        ogs_debug( "response: %s", response.content());
                        nf_profile = cJSON_Parse(response.content());
                        nfprofile = OpenAPI_nf_profile_parseFromJSON(nf_profile);
                        if (!nfprofile) {
                            ogs_error("No nf_profile");
                        }
                        message.nfProfile(nfprofile);
                        cJSON_Delete(nf_profile);
                    }

                    ogs_assert(OGS_FSM_STATE(&nf_instance->sm));

                    event.sbiMessage(message);
                    ogs_fsm_dispatch(&nf_instance->sm, event.ogsEvent());
                } else if (resource ==  OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS) {
                    ogs_sbi_subscription_data_t *subscription_data(reinterpret_cast<ogs_sbi_subscription_data_t*>(event.sbiData()));
                    ogs_assert(subscription_data);

                    std::string method(message.method());
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
                        if (message.resStatus() == OGS_SBI_HTTP_STATUS_CREATED ||
                            message.resStatus() == OGS_SBI_HTTP_STATUS_OK) {
                            ogs_nnrf_nfm_handle_nf_status_subscribe(
                                    subscription_data, message.ogsSBIMessage());
                        } else {
                            ogs_error("HTTP response error : %d", message.resStatus());
                        }
                    } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
                        if (message.resStatus() == OGS_SBI_HTTP_STATUS_NO_CONTENT) {
                            ogs_sbi_subscription_data_remove(subscription_data);
                        } else {
                            ogs_error("HTTP response error : %d", message.resStatus());
                        }
                    } else {
                            ogs_error("Invalid HTTP method [%s]", method.c_str());
                    }
                } else {
                    ogs_error("Invalid resource name [%s]", resource.c_str());
                }
            } else {
                ogs_error("Invalid service name [%s]", service_name.c_str());
                ogs_assert_if_reached();
            }
        }
        break;

    case OGS_EVENT_SBI_TIMER:
        {
            ogs_assert(event.ogsEvent());

            switch(event.timerId()) {
            case OGS_TIMER_NF_INSTANCE_REGISTRATION_INTERVAL:
            case OGS_TIMER_NF_INSTANCE_HEARTBEAT_INTERVAL:
            case OGS_TIMER_NF_INSTANCE_NO_HEARTBEAT:
            case OGS_TIMER_NF_INSTANCE_VALIDITY:
                {
                    ogs_sbi_nf_instance_t *nf_instance(reinterpret_cast<ogs_sbi_nf_instance_t*>(event.sbiData()));
                    ogs_assert(nf_instance);
                    ogs_assert(OGS_FSM_STATE(&nf_instance->sm));

                    ogs_fsm_dispatch(&nf_instance->sm, event.ogsEvent());
                    if (OGS_FSM_CHECK(&nf_instance->sm, ogs_sbi_nf_state_exception))
                        ogs_error("State machine exception [%d]", event.timerId());
                }
                break;

            case OGS_TIMER_SUBSCRIPTION_VALIDITY:
                {
                    ogs_sbi_subscription_data_t *subscription_data(reinterpret_cast<ogs_sbi_subscription_data_t*>(event.sbiData()));
                    ogs_assert(subscription_data);

                    ogs_assert(true ==
                            ogs_nnrf_nfm_send_nf_status_subscribe(
                            ogs_sbi_self()->nf_instance->nf_type,
                            subscription_data->req_nf_instance_id,
                            subscription_data->subscr_cond.nf_type,
                            subscription_data->subscr_cond.service_name));

                    ogs_debug("Subscription validity expired [%s]", subscription_data->id);
                    ogs_sbi_subscription_data_remove(subscription_data);
                }
                break;

            case OGS_TIMER_SBI_CLIENT_WAIT:
                {
                    ogs_sbi_xact_t *sbi_xact(reinterpret_cast<ogs_sbi_xact_t*>(event.sbiData()));
                    ogs_assert(sbi_xact);
                    Open5GSSBIStream stream(sbi_xact->assoc_stream);

                    ogs_sbi_xact_remove(sbi_xact);

                    ogs_error("Cannot receive SBI message");
                    if (stream) {
                        ogs_assert(true == Open5GSSBIServer::sendError(stream,
                                    OGS_SBI_HTTP_STATUS_GATEWAY_TIMEOUT, nullptr,
                                    "Cannot receive SBI message", nullptr));
                    }
                }
                break;

            default:
                ogs_error("Unknown timer[%s:%d]", ogs_timer_get_name(event.timerId()), event.timerId());
            }
        }
        break;

    default:
        ogs_error("No handler for event %s", ogs_event_get_name(event.ogsEvent()));
        break;
    }
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
