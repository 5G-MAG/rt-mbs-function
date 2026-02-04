#ifndef _MBSF_QOS_REQ_HH_
#define _MBSF_QOS_REQ_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Service Info class
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
#include "mb-smf-service-consumer.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "common.hh"
#include "openapi/model/Arp.h"
#include "openapi/model/MbsQoSReq.h"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class QoSReq {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    QoSReq(fiveg_mag_reftools::CJson &json, bool as_request);
    QoSReq(const std::shared_ptr<reftools::mbsf::MbsQoSReq> &mbs_qos_req);
    QoSReq() = delete;
    QoSReq(QoSReq &&other) = delete;
    QoSReq(const QoSReq &other) = delete;
    QoSReq &operator=(QoSReq &&other) = delete;
    QoSReq &operator=(const QoSReq &other) = delete;

    virtual ~QoSReq();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::MbsQoSReq> &getMBSQoSReq() const {return m_mbsQoSReq;};
    const int32_t getQosReqR5qi() const {return m_mbsQoSReq->getR5qi();};
    const std::optional<std::string> &getGuardBitRate() const { return m_mbsQoSReq->getGuarBitRate();};
    const std::optional<std::string> &getMaxBitRate() const { return m_mbsQoSReq->getMaxBitRate();};
    const std::optional<int32_t> getAverageWindow() const { return m_mbsQoSReq->getAverWindow();};
    const std::optional<std::shared_ptr<reftools::mbsf::Arp> > &getReqMbsArp() const { return m_mbsQoSReq->getReqMbsArp();};

    mb_smf_sc_mbs_qos_req_t *populateQoSReq();
    uint64_t *bitrate(const std::optional<std::string> &bit_rate);
    uint16_t *averagingWindow();

private:
    std::shared_ptr<reftools::mbsf::MbsQoSReq> m_mbsQoSReq;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_SERVICE_INFO_HH_ */
