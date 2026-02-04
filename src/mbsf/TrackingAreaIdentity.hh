#ifndef _MBSF_TAI_HH_
#define _MBSF_TAI_HH_
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "mb-smf-service-consumer.h"

#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/Tai.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class PlmnId;
}

MBSF_NAMESPACE_START

class TrackingAreaIdentity {
public:

    TrackingAreaIdentity(fiveg_mag_reftools::CJson &json, bool as_request);
    TrackingAreaIdentity(const std::shared_ptr<reftools::mbsf::Tai> &tai);
    TrackingAreaIdentity() = delete;
    TrackingAreaIdentity(TrackingAreaIdentity &&other) = delete;
    TrackingAreaIdentity(const TrackingAreaIdentity &other) = delete;
    TrackingAreaIdentity &operator=(TrackingAreaIdentity &&other) = delete;
    TrackingAreaIdentity &operator=(const TrackingAreaIdentity &other) = delete;

    virtual ~TrackingAreaIdentity();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::Tai> &getTai() const {return m_tai;};
    const std::shared_ptr<reftools::mbsf::PlmnId> &getPlmnId() const {return m_tai->getPlmnId();};
    const std::string &getTac() const {return m_tai->getTac();};
    const std::optional<std::string > &getNid() const {return m_tai->getNid();};

    uint32_t tac();
    uint64_t *nid();
    mb_smf_sc_tai_t *populateTai();

private:
    std::shared_ptr<reftools::mbsf::Tai> m_tai;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_TAI_HH_ */
