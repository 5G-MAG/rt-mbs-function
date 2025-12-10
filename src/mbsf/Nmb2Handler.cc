/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Nmb2 Handler
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
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "UserService.hh"
#include "UserDataIngSession.hh"
#include "openapi/model/CreateRspData.h"
#include "openapi/model/DistSession.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/Ipv6Addr.h"
#include "openapi/model/TunnelAddress.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/UpTrafficFlowInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/ProblemCause.hh"

#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"

#include "Nmb2Handler.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::CreateRspData;
using reftools::mbsf::DistSession;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::TunnelAddress;
using reftools::mbsf::Ssm;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MbsServiceType;
using reftools::mbsf::IpAddr;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::UpTrafficFlowInfo;
using reftools::mbsf::Ipv6Addr;

MBSF_NAMESPACE_START

static bool handle_mbstf_dist_session_response(ogs_sbi_xact_t *xact, Open5GSSBIResponse &response);
static void handle_mbstf_dist_session_delete(std::string &dist_session_id);
static void send_error(ogs_sbi_xact_t *xact);
static void remove_xact(ogs_sbi_xact_t *xact);
static bool valid_content_type(Open5GSSBIMessage &message);

bool Nmb2Handler::processEvent(Open5GSEvent &event)
{
    switch (event.id()) {
    case OGS_EVENT_SBI_CLIENT:
    {
        ogs_assert(event.ogsEvent());

        Open5GSSBIResponse response(event.sbiResponse());
        Open5GSSBIMessage message;

        try {
            message.parseHeader(response);
        } catch (std::exception &ex) {
            ogs_error("ogs_sbi_parse_response() failed decoding client response");
            response.setOwner(true);
            return true;
        }

        message.resStatus(response.status());

        std::string service_name(message.serviceName());
        if (service_name == OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION) {
            std::string resource(message.resourceComponent(0));
            if (resource == "dist-sessions") {
                response.setOwner(true);

                ogs_sbi_xact_t *sbi_xact = NULL;
                ogs_pool_id_t sbi_xact_id = 0;

                sbi_xact_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_xact_t*>(event.sbiData()));
                ogs_assert(sbi_xact_id >= OGS_MIN_POOL_ID && sbi_xact_id <= OGS_MAX_POOL_ID);

                sbi_xact = ogs_sbi_xact_find_by_id(sbi_xact_id);
                ogs_assert(sbi_xact);
                if (!sbi_xact) {
                    /* CLIENT_WAIT timer could remove SBI transaction
                     * before receiving SBI message */
                    ogs_error("SBI transaction has already been removed");
                    return true;
                }
                std::string method(message.method());
                if (method == OGS_SBI_HTTP_METHOD_POST) {
                    if (message.resStatus() == OGS_SBI_HTTP_STATUS_OK || message.resStatus() == OGS_SBI_HTTP_STATUS_CREATED)
                    {
                        if( response.contentLength() && valid_content_type(message))
                        {

                            handle_mbstf_dist_session_response(sbi_xact, response);
                        } else {
                            ogs_error("Received invalid content-type from MBSTF");
                            UserDataIngSession::deleteMBSTFSession(sbi_xact);
                            send_error(sbi_xact);
                         }

                    } else {
                        ogs_error("HTTP response error [%d]", message.resStatus());
                        UserDataIngSession::deleteMBSTFSession(sbi_xact);
                        send_error(sbi_xact);

                    }

                } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
                    if (message.resStatus() == OGS_SBI_HTTP_STATUS_NO_CONTENT)
                    {
                        std::string resource_id(message.resourceComponent(1));
                        if(!resource_id.empty()) {
                            if(sbi_xact) {
                                remove_xact(sbi_xact);
                                sbi_xact = nullptr;
                            }
                            handle_mbstf_dist_session_delete(resource_id);
                        }

                    }


                } else if (method == OGS_SBI_HTTP_METHOD_PATCH) {
                    ogs_info("PATCH MESSAGE STATUS: %d %lu CT:[%s]", message.resStatus(), response.contentLength(), message.contentType());
                     if (message.resStatus() == OGS_SBI_HTTP_STATUS_OK)
                    {
                        if( response.contentLength() && valid_content_type(message))
                        {
                            CJson patch_response_from_mbstf(CJson::Null);
                            try {
                                patch_response_from_mbstf = CJson::parse(response.content());
                            } catch (std::exception &ex) {
                                ogs_info("PROBLEM IN PARSING PATCH RESPONSE FROM MBSTF");
                            }

                            {
                                std::string txt(patch_response_from_mbstf.serialise());
                                ogs_debug("PATCH RESPONSE Parsed JSON: %s", txt.c_str());
                            }
                            UserDataIngSession::handlePatchUpdateResponse(sbi_xact);



                        }
                     } else {
                         ogs_error("MBSTF Patch Update failed");
                         UserDataIngSession::rollbackMBSTFDistSessionState(sbi_xact);
                     }


                } else {
                    ogs_error("Invalid HTTP method [%s]", method.c_str());
                }
                if(sbi_xact) {
                    remove_xact(sbi_xact);
                    sbi_xact = nullptr;
                }
                return true;
            }
            return false;
        }

        if(!response.owner()) response.resetHeader();
    }
    return false;
    default:
        return false;
    }
}

static bool handle_mbstf_dist_session_response(ogs_sbi_xact_t *xact, Open5GSSBIResponse &response)
{
    ogs_sbi_object_t *sbi_object = NULL;
    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NULL;

    OpenAPI_nf_type_e target_nf_type = OpenAPI_nf_type_NULL;
    OpenAPI_nf_type_e requester_nf_type = OpenAPI_nf_type_NULL;

    ogs_assert(xact);
    sbi_object = xact->sbi_object;
    ogs_assert(sbi_object);
    service_type = xact->service_type;
    ogs_assert(service_type);
    target_nf_type = ogs_sbi_service_type_to_nf_type(service_type);
    ogs_assert(target_nf_type);
    requester_nf_type = xact->requester_nf_type;
    ogs_assert(requester_nf_type);

    std::shared_ptr< CreateRspData > create_rsp_data = nullptr;
    std::shared_ptr< DistSession > dist_session = nullptr;

    CJson create_rsp_data_from_mbstf(CJson::Null);
    try {
        create_rsp_data_from_mbstf = CJson::parse(response.content());
    } catch (std::exception &ex) {
        UserDataIngSession::deleteMBSTFSession(xact);
        send_error(xact);
        return true;
    }
    try {
        create_rsp_data.reset(new CreateRspData(create_rsp_data_from_mbstf, false));

    } catch (std::exception &err) {
        UserDataIngSession::deleteMBSTFSession(xact);
        char *error = ogs_msprintf("%s", err.what());
        ogs_error("%s", error);
        send_error(xact);
        ogs_free(error);
        return true;
    }

    dist_session = create_rsp_data->getDistSession();
    ogs_expect(true == UserDataIngSession::processDistSession(dist_session));
    return true;
}

static bool valid_content_type(Open5GSSBIMessage &message) {
    if(std::string(message.contentType()) == "application/json") {
        return true;
    }
    return false;

}

static void send_error(ogs_sbi_xact_t *xact)
{

    Open5GSSBIStream stream(xact->assoc_stream_id);
    ogs_assert(true == Open5GSSBIServer::sendError(stream, std::nullopt, ProblemCause::INBOUND_SERVER_ERROR,
                                                                      "Failed to create MBS Distribution Session in MBSTF"));
}

static void remove_xact(ogs_sbi_xact_t *xact)
{
    if(xact) {
        UserDataIngSession::removeXact(xact);
        xact = nullptr;
    }
}


static void handle_mbstf_dist_session_delete(std::string &dist_session_id) {

    UserDataIngSession::setMBSTFDistSessionDeletedFlag(dist_session_id);

}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
