/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBSF Open5GS Event Handler
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
#include "MBSMFMBSSession.hh"
#include "Nmb2Handler.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"
#include "NfServer.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIStream.hh"
#include "UserService.hh"
#include "UserServiceAnnChannel.hh"
#include "UserDataIngSession.hh"
#include "UserDataIngStatSubsc.hh"
#include "openapi/model/ProblemCause.hh"

#include "MBSFEventHandler.hh"

using fiveg_mag_reftools::ProblemCause;

MBSF_NAMESPACE_START

static void mbsf_nnrf_handle_nf_discover(ogs_sbi_xact_t *xact, ogs_sbi_message_t *recvmsg);

void MBSFEventHandler::dispatch(Open5GSFSM &fsm, Open5GSEvent &event)
{
    // Handle Open5GS FSM events here

    if (UserService::processEvent(event)) return;
    if (UserDataIngStatSubsc::processEvent(event)) return;
    if (UserDataIngSession::processEvent(event)) return;
    if (Nmb2Handler::processEvent(event)) return;
    if (MBSMFMBSSession::processEvent(event)) return;
    if (UserServiceAnnChannel::processEvent(event)) return;
    if (mb_smf_sc_process_event(event.ogsEvent())) return;

    ogs_debug("MBSF Event: %s", ogs_event_get_name(event.ogsEvent()));

    switch (event.id()) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("[%s] MBSF Running", ogs_sbi_self()->nf_instance->id);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case OGS_EVENT_SBI_SERVER:
        {
            Open5GSSBIRequest request(event.sbiRequest());
            Open5GSSBIStream stream(OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData())));

            Open5GSSBIMessage message;

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("ogs_sbi_parse_header() failed");
                ogs_assert(true == Open5GSSBIServer::sendError(
                                stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                message, "Bad Request", "Cannot parse HTTP message", nullptr));
                break;
            }

            std::string service_name(message.serviceName());
            if (service_name == OGS_SBI_SERVICE_NAME_NNRF_NFM) {
                std::string api_version(message.apiVersion());
                if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Not supported version [%s]", api_version.c_str());
                    ogs_assert(true == Open5GSSBIServer::sendError(stream, message, ProblemCause::INVALID_API,
                                                                   "Not supported version"));
                    break;
                }
                std::string resource(message.resourceComponent(0));
                if (resource == OGS_SBI_RESOURCE_NAME_NF_STATUS_NOTIFY) {
                    std::string method(message.method());
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
                        /* Parsed here rather than before the resource and method are known, so a
                           request this callback does not serve is answered on its own terms instead
                           of on whatever its body happens to contain.

                           parseRequest() throws when the body does not satisfy the schema, and an
                           NRF is free to send one that does not. TS 29.500 V18.10.0 clause 5.2.7.2:
                           “If a received HTTP request contains IEs or query parameters not compliant
                           with the schema defined in the corresponding OpenAPI specification, the NF
                           should reject the request with the appropriate error code, e.g. "400 Bad
                           Request (INVALID_MSG_FORMAT)", even when the failed IEs are defined as
                           optional by the schema.” */
                        try {
                            message.parseRequest(request);
                        } catch (std::exception &ex) {
                            ogs_error("ogs_sbi_parse_request() failed on NRF status notification");
                            ogs_assert(true == Open5GSSBIServer::sendError(
                                            stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, message,
                                            "Bad Request", "Cannot parse NF status notification body",
                                            ProblemCause::INVALID_MSG_FORMAT.cause().c_str()));
                            break;
                        }
                        ogs_nnrf_nfm_handle_nf_status_notify(stream.ogsSBIStream(), message.ogsSBIMessage());
                    } else {
                        ogs_error("Invalid HTTP method [%s]", method.c_str());
                        /* Answered through NfServer so the response carries the Allow header, which
                           Open5GSSBIServer::sendError cannot add: it hands the response to
                           ogs_sbi_server_send_error(), which builds and sends it internally.

                           TS 29.500 V18.10.0 clause 5.2.7.2: “If the NF supports the HTTP method for
                           several resources in the API, but not for the target resource of a given HTTP
                           request, the NF shall reject the request with the HTTP status code "405 Method
                           Not Allowed" and shall include in the response an Allow header field
                           containing the supported method(s) for that resource.”

                           This resource is the NRF's status notification callback, which serves POST
                           and nothing else. */
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_METHOD_NOT_ALLOWED, 0,
                                        message, App::self().mbsfAppMetadata(), std::nullopt,
                                        "Method Not Allowed", "Invalid HTTP method in NRF status notification",
                                        std::nullopt, std::nullopt, std::nullopt,
                                        std::string(OGS_SBI_HTTP_METHOD_POST)));
                    }
                } else {
                    ogs_error("Invalid resource name [%s]", resource.c_str());
                    ogs_assert(true == Open5GSSBIServer::sendError(stream, message, ProblemCause::RESOURCE_URI_STRUCTURE_NOT_FOUND,
                                                                    "Invalid resource name"));
                }
            } else {
                ogs_error("Invalid API name [%s]", service_name.c_str());
                ogs_assert(true == Open5GSSBIServer::sendError(stream, message, ProblemCause::INVALID_API, "Invalid API name"));
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
                ogs_error("ogs_sbi_parse_response() failed decoding client response");
                break;
            }

            message.resStatus(response.status());

            std::string service_name(message.serviceName());
            if (service_name == OGS_SBI_SERVICE_NAME_NNRF_DISC) {
                std::string resource(message.resourceComponent(0));
                if (resource == OGS_SBI_RESOURCE_NAME_NF_INSTANCES) {
                    ogs_sbi_xact_t *sbi_xact = NULL;
                    ogs_pool_id_t sbi_xact_id = 0;

                    message.parseResponse(response);

                    sbi_xact_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_xact_t*>(event.sbiData()));
                    ogs_assert(sbi_xact_id >= OGS_MIN_POOL_ID && sbi_xact_id <= OGS_MAX_POOL_ID);

                    sbi_xact = ogs_sbi_xact_find_by_id(sbi_xact_id);
                    if (!sbi_xact) {
                          /* CLIENT_WAIT timer could remove SBI transaction
                           * before receiving SBI message */
                          ogs_error("SBI transaction has already been removed [%d]", sbi_xact_id);
                          break;
                    }
                    std::string method(message.method());
                    if (method == OGS_SBI_HTTP_METHOD_GET) {
                        if (message.resStatus() == OGS_SBI_HTTP_STATUS_OK)
                        {

                            mbsf_nnrf_handle_nf_discover(sbi_xact, message.ogsSBIMessage());
                        } else {
                            ogs_error("HTTP response error [%d]", message.resStatus());
                        }

                    } else {
                          ogs_error("Invalid HTTP method [%s]", method.c_str());
                          ogs_assert_if_reached();
                    }

                    if (sbi_xact) UserDataIngSession::removeXact(sbi_xact);
                    sbi_xact = NULL;
                }

            } else if (service_name == OGS_SBI_SERVICE_NAME_NNRF_NFM) {
                std::string resource(message.resourceComponent(0));
                message.parseResponse(response);
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
                // This generic SBI-client dispatch branch has cases only for OGS_SBI_SERVICE_NAME_NNRF_NFM and
                // NNRF_DISC (see the enclosing if/else chain), but MBSF legitimately calls other services and so
                // receives their responses here too: "nmbsmf-mbssession", MB-SMF's own session service, arrives
                // on Broadcast Context Create and Release. An unexpected service name is therefore logged and
                // ignored, matching the "Invalid resource name" and "Invalid HTTP method" cases just above,
                // rather than reaching ogs_assert_if_reached() and taking the process down.
                ogs_error("Invalid service name [%s]", service_name.c_str());
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
                    ogs_debug("OGS_EVENT_SBI_TIMER [%d]", event.timerId());
                    ogs_sbi_nf_instance_t *nf_instance(reinterpret_cast<ogs_sbi_nf_instance_t*>(event.sbiData()));
                    ogs_assert(nf_instance);
                    ogs_assert(OGS_FSM_STATE(&nf_instance->sm));

                    ogs_sbi_self()->nf_instance->load = App::self().context()->load();

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
                    ogs_sbi_xact_t *sbi_xact = NULL;
                    ogs_pool_id_t sbi_xact_id = 0;

                    sbi_xact_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_xact_t*>(event.sbiData()));
                    ogs_assert(sbi_xact_id >= OGS_MIN_POOL_ID && sbi_xact_id <= OGS_MAX_POOL_ID);

                    sbi_xact = ogs_sbi_xact_find_by_id(sbi_xact_id);
                    if (!sbi_xact) {
                          /* A response and this timer's expiry can be queued in the same poll, and
                             the response frees the transaction before the expiry is handled. The
                             lookup is what tells the two apart; see the AMF's own account of it at
                             subprojects/open5gs/src/amf/amf-sm.c:771. */
                          ogs_error("SBI transaction has already been removed [%d]", sbi_xact_id);
                          break;
                    }

                    /* An Nmb2 create that never gets an answer is the MBSF failing to establish the
                       Distribution Session at the MBSTF just as much as a rejected one is. The call
                       filters on the transaction's own request, so transactions for anything else
                       pass through it untouched. */
                    UserDataIngSession::registerDistSessionEstFailure(sbi_xact,
                            "MBSTF did not answer the MBS Distribution Session creation");

                    /* A consumer DELETE waits on a stream this transaction does not name:
                       assoc_stream_id still carries the stream the Distribution Session was
                       created on, closed long before any delete, so the assoc_stream path below
                       finds nothing and, until this call, returned having answered nobody. The
                       consumer was then left holding a DELETE that no later event could complete,
                       because the transaction it depended on had just been destroyed. */
                    if (UserDataIngSession::failPendingDeleteRequests(sbi_xact, ProblemCause::UNSPECIFIED_NF_FAILURE,
                                                                      "No response from the downstream NF")) {
                        ogs_error("Cannot receive SBI message");
                        UserDataIngSession::removeXact(sbi_xact);
                        return;
                    }

                    ogs_sbi_stream_t *ogs_stream = reinterpret_cast<ogs_sbi_stream_t*>(ogs_sbi_stream_find_by_id(sbi_xact->assoc_stream_id));
                    if (!ogs_stream) {
                        if (sbi_xact) UserDataIngSession::removeXact(sbi_xact);
                        return;
                     }

                    Open5GSSBIStream stream(sbi_xact->assoc_stream_id);
                    if (sbi_xact) UserDataIngSession::removeXact(sbi_xact);
                    ogs_error("Cannot receive SBI message");
                    if (stream) {
                        /* This timer is the MBSF's own client wait timer, so what expired is the
                           request the MBSF sent downstream, not the request it is answering.
                           TIMED_OUT_REQUEST names the other situation: TS 29.500 V18.10.0
                           table 5.2.7.2-1 defines it as "The request is rejected due a request that
                           has timed out at the HTTP client (see clause 6.11.2)", clause 6.11.2 being
                           the 3gpp-Sbi-Max-Rsp-Time mechanism by which a server learns that its own
                           consumer has already given up. Nothing in that table covers a downstream
                           peer that never answered -- INBOUND_SERVER_ERROR is scoped by clause
                           6.4.2.1 to a 503 or 429 actually received -- which is the case its NOTE 3
                           reserves UNSPECIFIED_NF_FAILURE for. */
                        ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, ProblemCause::UNSPECIFIED_NF_FAILURE,
                                                                      "No response from the downstream NF"));
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

static void mbsf_nnrf_handle_nf_discover(ogs_sbi_xact_t *xact, ogs_sbi_message_t *recvmsg)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_object_t *sbi_object = NULL;
    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NULL;
    ogs_sbi_discovery_option_t *discovery_option = NULL;

    OpenAPI_nf_type_e target_nf_type = OpenAPI_nf_type_NULL;
    OpenAPI_nf_type_e requester_nf_type = OpenAPI_nf_type_NULL;
    OpenAPI_search_result_t *SearchResult = NULL;

    ogs_assert(recvmsg);
    ogs_assert(xact);
    sbi_object = xact->sbi_object;
    ogs_assert(sbi_object);
    service_type = xact->service_type;
    ogs_assert(service_type);
    target_nf_type = ogs_sbi_service_type_to_nf_type(service_type);
    ogs_assert(target_nf_type);
    requester_nf_type = xact->requester_nf_type;
    ogs_assert(requester_nf_type);

    discovery_option = xact->discovery_option;


    SearchResult = recvmsg->SearchResult;
    if (!SearchResult) {
        ogs_error("No SearchResult");
        return;
    }

    ogs_nnrf_disc_handle_nf_discover_search_result(SearchResult);

    nf_instance = ogs_sbi_nf_instance_find_by_discovery_param(
                    target_nf_type, requester_nf_type, discovery_option);
    if (!nf_instance) {
        ogs_error("(NF discover) No [%s:%s]",
                    ogs_sbi_service_type_to_name(service_type),
                    OpenAPI_nf_type_ToString(requester_nf_type));
        //return;
    }
    ogs_expect(true == UserDataIngSession::handleMbstfDiscover(nf_instance, xact));
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
