#ifndef _MBSF_ASSOCIATED_SESSION_ID_HH_
#define _MBSF_ASSOCIATED_SESSION_ID_HH_
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
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/IpAddr.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class AssociatedSessId {
public:

    AssociatedSessId(fiveg_mag_reftools::CJson &json, bool as_request);
    AssociatedSessId(const std::shared_ptr<reftools::mbsf::AssociatedSessionId> &associated_session_id);
    AssociatedSessId() = delete;
    AssociatedSessId(AssociatedSessId &&other) = delete;
    AssociatedSessId(const AssociatedSessId &other) = delete;
    AssociatedSessId &operator=(AssociatedSessId &&other) = delete;
    AssociatedSessId &operator=(const AssociatedSessId &other) = delete;

    virtual ~AssociatedSessId();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::AssociatedSessionId> &getAssociatedSessionId() const {return m_associatedSessionId;};
    const std::shared_ptr<reftools::mbsf::IpAddr> &getSourceIpAddr() const {return m_associatedSessionId->getSourceIpAddr();};
    const std::shared_ptr<reftools::mbsf::IpAddr> &getDestIpAddr() const {return m_associatedSessionId->getDestIpAddr();};

    mb_smf_sc_associated_session_id_t *populateAssociatedSessionId();
    void populateSsm(mb_smf_sc_associated_session_id_t *session_id);

private:
    std::shared_ptr<reftools::mbsf::AssociatedSessionId> m_associatedSessionId;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_ASSOCIATED_SESSION_ID_HH_ */
