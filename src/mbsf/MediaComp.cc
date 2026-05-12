/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Media Comp class
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
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <iostream>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "MediaInfo.hh"
#include "QoSReq.hh"
#include "UserDataIngSession.hh"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/MbsMediaCompRm.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/ObjectDistrMethInfo.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "MediaComp.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsMediaCompRm;
using reftools::mbsf::MbsMediaInfo;
using reftools::mbsf::MbsQoSReq;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::ReservPriority;

MBSF_NAMESPACE_START

MediaComp::MediaComp(CJson &json, bool as_request)
    :m_mbsMediaComp(new MbsMediaCompRm(json, as_request))
    ,m_mediaInfo(nullptr)
    ,m_qosReq(nullptr)
{
}

MediaComp::MediaComp(const std::shared_ptr<MbsMediaCompRm> &mbs_media_comp)
    :m_mbsMediaComp(mbs_media_comp)
    ,m_mediaInfo(nullptr)
    ,m_qosReq(nullptr)
{
}

MediaComp::~MediaComp()
{
}

CJson MediaComp::json(bool as_request = false) const
{
    return m_mbsMediaComp->toJSON(as_request);
}


std::list<std::optional<std::string >> MediaComp::getMbsFlowDescs() {
    reftools::mbsf::MbsMediaCompRm::MbsFlowDescsType mbs_flow_descs = m_mbsMediaComp->getMbsFlowDescs();
    std::optional<std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > > flow_descs = *mbs_flow_descs;
    if (!flow_descs.has_value()) {
       return {};
    }
    return std::list<std::optional<std::string >>(flow_descs->begin(), flow_descs->end());

}

ogs_list_t *MediaComp::flowDescriptions() {

    //std::optional<std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > > MbsFlowDescsType;
    const reftools::mbsf::MbsMediaCompRm::MbsFlowDescsType &mbs_flow_descs = m_mbsMediaComp->getMbsFlowDescs();
    if (!mbs_flow_descs.has_value()) return nullptr;

    ogs_list_t *flow_desc = nullptr;

    flow_desc = (ogs_list_t*) ogs_calloc(1,sizeof(*flow_desc));
    ogs_assert(flow_desc);
    ogs_list_init(flow_desc);

    for(const auto &mbs_flow_desc: mbs_flow_descs.value()) {
        if (mbs_flow_desc.has_value()) {
            mb_smf_sc_flow_description_t *flow_desc_node = mb_smf_sc_flow_description_new();
            ogs_assert(flow_desc_node);
            const std::string &mbs_flow_desc_val = mbs_flow_desc.value();
            flow_desc_node->string = ogs_strdup(mbs_flow_desc_val.c_str());
            ogs_list_add(flow_desc, flow_desc_node);
        }

    }
    return flow_desc;

}

mb_smf_sc_mbs_media_comp_t *MediaComp::populateMediaComp() {
    std::shared_ptr< MediaInfo > info = nullptr;
    std::shared_ptr< QoSReq > qos_req = nullptr;
    mb_smf_sc_mbs_media_comp_t *media_comp = mb_smf_sc_mbs_media_comp_new();
    media_comp->id = getId();
    media_comp->flow_descriptions = flowDescriptions();
    const std::optional<std::shared_ptr< ReservPriority > > &reserv_priority =  getMbsSdfResPrio();
    if (reserv_priority.has_value()) {
        std::shared_ptr< ReservPriority > priority = reserv_priority.value();
        media_comp->mbs_sdf_reserve_priority = static_cast<uint8_t>(priority->getValue());
    } else {
        media_comp->mbs_sdf_reserve_priority = 0;
    }
    const std::optional<std::shared_ptr< MbsMediaInfo > >  &media_info = getMbsMediaInfo();
    if (media_info.has_value()) {
        info.reset(new MediaInfo(media_info.value()));
        media_comp->media_info = info->populateMediaInfo();
    } else {
        media_comp->media_info = nullptr;
    }
    const std::optional<std::string > &qos_ref = getQosRef();
    if (qos_ref.has_value()) {

        media_comp->qos_ref = ogs_strdup(qos_ref.value().c_str());
    } else {

        media_comp->qos_ref = nullptr;
    }
    const std::optional<std::shared_ptr< MbsQoSReq > > &mbs_qos_req = getMbsQoSReq();
    if (mbs_qos_req.has_value()) {
        qos_req.reset(new QoSReq(mbs_qos_req.value()));
        media_comp->mbs_qos_req = qos_req->populateQoSReq();
    } else {
        media_comp->mbs_qos_req = nullptr;
    }
    return media_comp;
}

#if 0
typedef struct mb_smf_sc_mbs_media_comp_s {
    int id;
    ogs_list_t *flow_descriptions;
    uint8_t mbs_sdf_reserve_priority; /**< MBS SDF Reserve Priority (1-16 inclusive or 0 to unset) */
    mb_smf_sc_mbs_media_info_t *media_info;
    char *qos_ref;
    mb_smf_sc_mbs_qos_req_t *mbs_qos_req;
} mb_smf_sc_mbs_media_comp_t;
#endif

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

