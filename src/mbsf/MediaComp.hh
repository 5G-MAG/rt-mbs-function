#ifndef _MBSF_MEDIA_COMP_HH_
#define _MBSF_MEDIA_COMP_HH_
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/MbsMediaCompRm.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class ReservPriority;
}

MBSF_NAMESPACE_START

class MediaInfo;
class QoSReq;

class MediaComp {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    MediaComp(fiveg_mag_reftools::CJson &json, bool as_request);
    MediaComp(const std::shared_ptr<reftools::mbsf::MbsMediaCompRm> &mbs_media_comp);
    MediaComp() = delete;
    MediaComp(MediaComp &&other) = delete;
    MediaComp(const MediaComp &other) = delete;
    MediaComp &operator=(MediaComp &&other) = delete;
    MediaComp &operator=(const MediaComp &other) = delete;

    virtual ~MediaComp();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::MbsMediaCompRm> &getMbsMediaComp() const { return m_mbsMediaComp; };
    std::list<std::optional<std::string> > getMbsFlowDescs();
    const int32_t getId() const { return m_mbsMediaComp->getMbsMedCompNum(); };
    const std::optional<std::shared_ptr<reftools::mbsf::ReservPriority> > &getMbsSdfResPrio() const { return m_mbsMediaComp->getMbsSdfResPrio();};
    const std::optional<std::shared_ptr<reftools::mbsf::MbsMediaInfo> >  &getMbsMediaInfo() const { return m_mbsMediaComp->getMbsMediaInfo();};
    const std::optional<std::string> &getQosRef() const {return m_mbsMediaComp->getQosRef();};
    const std::optional<std::shared_ptr<reftools::mbsf::MbsQoSReq> > &getMbsQoSReq() const {return m_mbsMediaComp->getMbsQoSReq();};

    ogs_list_t *flowDescriptions();
    mb_smf_sc_mbs_media_comp_t *populateMediaComp();

private:
    std::shared_ptr<reftools::mbsf::MbsMediaCompRm> m_mbsMediaComp;
    std::shared_ptr<MediaInfo> m_mediaInfo;
    std::shared_ptr<QoSReq>  m_qosReq;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_MEDIA_COMP_HH_ */
