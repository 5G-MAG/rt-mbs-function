/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Nmb2 Build class
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
#include <random>
#include <uuid/uuid.h>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "hash.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "NfServer.hh"
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
#include "openapi/model/CreateReqData.h"
#include "openapi/model/DistSession.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/Ipv6Addr.h"
#include "openapi/model/TunnelAddress.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/UpTrafficFlowInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/PktDistributionData.h"

#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"
#include "mb-smf-service-consumer.h"
#include "TimerFunc.hh"

#include "UserDataIngSession.hh"

// Header include for this class

#include "Nmb2Build.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::CreateReqData;
using reftools::mbsf::DistributionMethod;
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
using reftools::mbsf::ObjDistributionData;
using reftools::mbsf::UpTrafficFlowInfo;
using reftools::mbsf::Ipv6Addr;
using reftools::mbsf::PktDistributionData;
using reftools::mbsf::PktDistributionOperatingMode;
using reftools::mbsf::PacketDistrMethInfo;
using reftools::mbsf::PktIngestMethod;
using reftools::mbsf::MbStfIngestAddr;

MBSF_NAMESPACE_START

static std::shared_ptr< ObjDistributionData > populate_mbstf_obj_distribution_data(std::shared_ptr<MBSDistributionSessionInfo> mbs_dist_session_info);
static std::shared_ptr< TunnelAddress > populate_mbstf_mb_upf_tunnel_addr(ogs_sockaddr_t *tunnel_addr);
static std::shared_ptr< UpTrafficFlowInfo > populate_mbstf_up_traffic_flow_info(std::shared_ptr< IpAddr > dest_addr);
static std::shared_ptr< DistSessionState > populate_mbstf_dist_session_state(std::shared_ptr<UserDataIngSession> ing_session, std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr);
static std::optional<std::shared_ptr< PktDistributionData > >  populate_mbstf_pkt_distribution_data(std::shared_ptr<MBSDistributionSessionInfo> mbs_dist_session_info);
static std::shared_ptr< DistSession > build_nmb2_create_dist_session(std::shared_ptr<UserDataIngSession> ing_session, std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr);
static std::string generate_uuid();

ogs_sbi_request_t *Nmb2Build::buildNmb2DistSession(void *context, void *data) {

    ogs_sbi_message_t msg;

    std::shared_ptr< ObjDistributionData > mbstf_obj_distribution_data = nullptr;
    std::optional<std::shared_ptr< PktDistributionData > > mbstf_pkt_distribution_data = std::nullopt;
    std::shared_ptr< UpTrafficFlowInfo > mbstf_up_traffic_flow_info = nullptr;
    std::shared_ptr< TunnelAddress > mbstf_mb_upf_tunnel_addr = nullptr;
    std::shared_ptr< DistSessionState > dist_session_state = nullptr;

    std::shared_ptr< DistSession > dist_session = nullptr;

    std::shared_ptr< CreateReqData > create_req_data = nullptr;
    memset(&msg, 0, sizeof(msg));
    msg.h.method = const_cast<char*>(OGS_SBI_HTTP_METHOD_POST);
    msg.h.service.name = const_cast<char*>("nmbstf-distsession");
    msg.h.api.version = const_cast<char*>("v1");
    msg.h.resource.component[0] = const_cast<char*>("dist-sessions");

    UserDataIngSession::UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngSession::UserDataIngDistSessId*>(context);
    std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids_ptr(ids);
    //std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids_ptr = std::make_shared<UserDataIngSession::UserDataIngDistSessId>(*ids);

    try {

        std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids_ptr->first);
        std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr(ing_session->getDistributionSessionInfoData(ids_ptr->second));

        create_req_data.reset(new CreateReqData());
        std::shared_ptr< DistSession > dist_session = build_nmb2_create_dist_session(ing_session, context_data_ptr);

        UserDataIngSession::setDistSessionId(context_data_ptr, generate_uuid());
        std::string sess_id(context_data_ptr->mbstfDistSessionId);

        dist_session->setDistSessionId(std::string(context_data_ptr->mbstfDistSessionId));

        create_req_data->setDistSession(std::move(dist_session));

        UserDataIngSession::addToRegistry(sess_id, ids_ptr);

        CJson json = create_req_data->toJSON(true);
        std::string body = std::string(json.serialise());

        ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);
        ogs_sbi_header_set(req->http.headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
        req->http.content = ogs_strdup(body.c_str());
        req->http.content_length = body.size();

        return req;
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << ids_ptr->first << "] does not exist.";
        ogs_error("%s", err.str().c_str());
    }
    return nullptr;

}

ogs_sbi_request_t *Nmb2Build::buildNmb2DistSessionDelete(void *context, void *data) {

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_DELETE;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)"dist-sessions";

    auto *session_ids = reinterpret_cast<UserDataIngSession::SessionIdContainer*>(data);

    const std::string &dist_session_id = std::string(session_ids->first);

    msg.h.resource.component[1] = const_cast<char*>(dist_session_id.c_str());

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

    delete session_ids;
    return req;

}


ogs_sbi_request_t *Nmb2Build::buildNmb2DistSessionPatch(void *context, void *data)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;
    CJson patch_val(CJson::Null);

    OpenAPI_list_t *patch_item_list = NULL;
    OpenAPI_patch_item_t status_item;

    memset(&status_item, 0, sizeof(status_item));

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PATCH;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] = (char *)"dist-sessions";

    auto *session_ids = reinterpret_cast<UserDataIngSession::SessionIdContainer*>(data);
    const std::string &dist_session_id = std::string(session_ids->first);

    message.h.resource.component[1] = const_cast<char*>(dist_session_id.c_str());

    message.http.content_type = (char *)OGS_SBI_CONTENT_PATCH_TYPE;

    std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(session_ids->second->first);
    std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr(ing_session->getDistributionSessionInfoData(session_ids->second->second));
    if(context_data_ptr->needsUpdate) {
        status_item.path = (char *)"/distSession";
        std::shared_ptr< DistSession > dist_session = build_nmb2_create_dist_session(ing_session, context_data_ptr);

        std::string sess_id(context_data_ptr->mbstfDistSessionId);

        dist_session->setDistSessionId(sess_id);
        UserDataIngSession::addToRegistry(sess_id, session_ids->second);

        patch_val = dist_session->toJSON(true);
    } else if(context_data_ptr->stateUpdate) {
        //patch_val = ing_session->distSessionState();
        patch_val = ing_session->getdistSessState().toJSON();
        ing_session->currentDistSessionState(ing_session->distSessionState());
        status_item.path = (char *)"/distSession/distSessionState";
    }

    patch_item_list = OpenAPI_list_create();
    if (!patch_item_list) {
        ogs_error("No patch_item_list");
    }

    status_item.op = OpenAPI_patch_operation_replace;
    cJSON *value = patch_val.exportCJSON();

    if(value)status_item.value = OpenAPI_any_type_create(value);
    if (!status_item.value) {
        ogs_error("No status item.value");
    }

    OpenAPI_list_add(patch_item_list, &status_item);

    message.PatchItemList = patch_item_list;

    request = ogs_sbi_build_request(&message);
    ogs_expect(request);

    if (status_item.value)
        OpenAPI_any_type_free(status_item.value);
    if (patch_item_list)
        OpenAPI_list_free(patch_item_list);

    delete session_ids;
    return request;
}

std::shared_ptr< UpTrafficFlowInfo > populate_mbstf_up_traffic_flow_info(std::shared_ptr< IpAddr > dest_addr)
{
    std::shared_ptr< UpTrafficFlowInfo > flow_info = nullptr;

    static std::random_device rd;
    static std::uniform_int_distribution<int32_t> ud(32768, 65535);
    int32_t port = ud(rd);

    flow_info.reset(new UpTrafficFlowInfo() );
    flow_info->setDestIpAddr(dest_addr);
    flow_info->setPortNumber(port);

    return flow_info;
}

std::shared_ptr < TunnelAddress > populate_mbstf_mb_upf_tunnel_addr(ogs_sockaddr_t *tunnel_addr)
{
    ogs_sockaddr_t *sa;
    char buf[OGS_ADDRSTRLEN + 1];

    std::shared_ptr< TunnelAddress > tun_addr(new TunnelAddress());

    for (sa = tunnel_addr; sa; sa = sa->next) {
        if(sa->ogs_sa_family == AF_INET) {
          OGS_ADDR(sa, buf);
          std::string ipv4_addr = std::string(buf, strnlen(buf, sizeof(buf)));
          tun_addr->setIpv4Addr(ipv4_addr);
          ogs_info("  UDP Tunnel = %s:%u", buf, OGS_PORT(sa));
          ogs_info("Recieved IPv4 tunnel address");
        } else if(sa->ogs_sa_family == AF_INET6) {
            std::shared_ptr < Ipv6Addr > ipv6_addr;

            OGS_ADDR(sa, buf);

            ipv6_addr.reset(new Ipv6Addr(buf, std::string::size_type(strnlen(buf, sizeof(buf)))));

            tun_addr->setIpv6Addr(std::make_optional(ipv6_addr));
        }
        tun_addr->setPortNumber(OGS_PORT(sa));
    }
    return tun_addr;
}

static std::shared_ptr< ObjDistributionData > populate_mbstf_obj_distribution_data(std::shared_ptr<MBSDistributionSessionInfo> mbs_dist_session_info)
{

    std::shared_ptr< ObjDistributionOperatingMode > operating_mode = UserDataIngSession::getOperatingMode(mbs_dist_session_info);

    std::shared_ptr< ObjAcquisitionMethod > acquisition_method = UserDataIngSession::getAcquisitionMethod(mbs_dist_session_info);

    std::optional<std::string> obj_ingest_url = UserDataIngSession::getObjectIngestUrl(mbs_dist_session_info);

    std::optional<std::string> obj_disribution_url = UserDataIngSession::getObjectDistributionUrl(mbs_dist_session_info);

    std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > >
            obj_acquisition_ids = UserDataIngSession::getObjectAcquisitionIds(mbs_dist_session_info);


    std::shared_ptr< ObjDistributionData > mbstf_obj_dist_data = nullptr;
    mbstf_obj_dist_data.reset(new ObjDistributionData());

    mbstf_obj_dist_data->setObjDistributionOperatingMode(operating_mode);
    mbstf_obj_dist_data->setObjAcquisitionMethod(acquisition_method);
    mbstf_obj_dist_data->setObjIngestBaseUrl(obj_ingest_url);
    mbstf_obj_dist_data->setObjDistributionBaseUrl(obj_disribution_url);
    if(acquisition_method) { 
        if(acquisition_method->getString() == "PULL") {
            mbstf_obj_dist_data->setObjAcquisitionIdsPull(obj_acquisition_ids);
        } else if(acquisition_method->getString() == "PUSH") {
            if (!obj_acquisition_ids.empty()) {
                mbstf_obj_dist_data->setObjAcquisitionIdPush(obj_acquisition_ids.front());
            }
        }
    }

    return mbstf_obj_dist_data;

}

static std::shared_ptr< DistSessionState > populate_mbstf_dist_session_state(std::shared_ptr<UserDataIngSession> ing_session, std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr)
{
    std::optional<std::shared_ptr< DistSessionState > > dist_session_state = context_data_ptr->info->getMbsDistSessState();
    if(dist_session_state.has_value()) {
        return dist_session_state.value();
    }
    return ing_session->getDistSessionState();
}

static std::optional<std::shared_ptr< PktDistributionData > >  populate_mbstf_pkt_distribution_data(std::shared_ptr<MBSDistributionSessionInfo> mbs_dist_session_info)
{

    std::shared_ptr< PktDistributionData > mbstf_pkt_dist_data = nullptr;
    std::optional<std::shared_ptr< PacketDistrMethInfo > > pkt_distr_meth_info = UserDataIngSession::getPktDistributionInfo(mbs_dist_session_info);
    if(!pkt_distr_meth_info.has_value()) return std::nullopt;
    std::shared_ptr< PktDistributionOperatingMode > operating_mode = UserDataIngSession::getPktDistributionOperatingMode(mbs_dist_session_info);
    std::shared_ptr< PktIngestMethod > pkt_ingest_method = UserDataIngSession::getPktIngestMethod(mbs_dist_session_info);
    std::shared_ptr< MbStfIngestAddr > ingest_addr = UserDataIngSession::getMbstfIngestAddr(mbs_dist_session_info);

    mbstf_pkt_dist_data.reset(new PktDistributionData());
    mbstf_pkt_dist_data->setPktDistributionOperatingMode(operating_mode);
    mbstf_pkt_dist_data->setPktIngestMethod(pkt_ingest_method);
    mbstf_pkt_dist_data->setMbStfIngestAddr(ingest_addr);
    return mbstf_pkt_dist_data;

}

static std::shared_ptr< DistSession > build_nmb2_create_dist_session(std::shared_ptr<UserDataIngSession> ing_session, std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr)
{
    std::shared_ptr< ObjDistributionData > mbstf_obj_distribution_data = nullptr;
    std::optional<std::shared_ptr< PktDistributionData > > mbstf_pkt_distribution_data = std::nullopt;
    std::shared_ptr< UpTrafficFlowInfo > mbstf_up_traffic_flow_info = nullptr;
    std::shared_ptr< TunnelAddress > mbstf_mb_upf_tunnel_addr = nullptr;
    std::shared_ptr< DistSessionState > dist_session_state = nullptr;

    std::shared_ptr< DistSession > dist_session = nullptr;

    std::shared_ptr< CreateReqData > create_req_data = nullptr;

    std::shared_ptr< DistributionMethod > distribution_method = context_data_ptr->distributionSessionInfo->getDistributionMethod();
    if(distribution_method && distribution_method->getString() == "OBJECT") {
        mbstf_obj_distribution_data = populate_mbstf_obj_distribution_data(context_data_ptr->info);
    }
    if(distribution_method && distribution_method->getString() == "PACKET") {
        mbstf_pkt_distribution_data = populate_mbstf_pkt_distribution_data(context_data_ptr->info);
    }
    std::shared_ptr< IpAddr  > dest_addr = context_data_ptr->ssm->getDestIpAddr();
    mbstf_up_traffic_flow_info = populate_mbstf_up_traffic_flow_info(dest_addr);

    ogs_sockaddr_t *tunnel_addr = context_data_ptr->MBSSession->tunnelAddr();
    mbstf_mb_upf_tunnel_addr = populate_mbstf_mb_upf_tunnel_addr(tunnel_addr);

    dist_session_state = populate_mbstf_dist_session_state(ing_session, context_data_ptr);

    std::string mbr = UserDataIngSession::maxContBitRate(context_data_ptr->info);
    //std::shared_ptr< DistSessionState > dist_sess_state = ing_session->getDistSessionState();

    dist_session.reset(new DistSession());
    if(mbstf_obj_distribution_data) dist_session->setObjDistributionData(mbstf_obj_distribution_data);
    if(mbstf_pkt_distribution_data) dist_session->setPktDistributionData(mbstf_pkt_distribution_data);
    dist_session->setUpTrafficFlowInfo(mbstf_up_traffic_flow_info);
    dist_session->setMbUpfTunAddr(mbstf_mb_upf_tunnel_addr);
    dist_session->setDistSessionState(dist_session_state);
    dist_session->setMbr(mbr);
    dist_session->setMaxDelay(context_data_ptr->info->getMaxContDelay());
    dist_session->setFecInformation(context_data_ptr->info->getFecConfig());
    dist_session->setDscpMarking(context_data_ptr->info->getTrafficMarkingInfo());

    ing_session->currentDistSessionState(*dist_session_state);

    return dist_session;
}

static std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
