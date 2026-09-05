#ifndef _MBSF_PLMN_ID_HH_
#define _MBSF_PLMN_ID_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS PlmnId class
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
#include "openapi/model/PlmnId.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class MBSPlmnId {
public:

    MBSPlmnId(fiveg_mag_reftools::CJson &json, bool as_request);
    MBSPlmnId(const std::shared_ptr<reftools::mbsf::PlmnId> &plmn_id);
    MBSPlmnId() = delete;
    MBSPlmnId(MBSPlmnId &&other) = delete;
    MBSPlmnId(const MBSPlmnId &other) = delete;
    MBSPlmnId &operator=(MBSPlmnId &&other) = delete;
    MBSPlmnId &operator=(const MBSPlmnId &other) = delete;

    virtual ~MBSPlmnId();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::PlmnId> &getPlmnId() const {return m_plmnId;};
    const std::string &getMcc() const {return m_plmnId->getMcc();};
    const std::string &getMnc() const {return m_plmnId->getMnc();};

    uint16_t mcc();
    uint16_t mnc();
    // The MNC's actual digit count (2 or 3), taken directly from the source
    // string rather than guessed from mnc()'s numeric value -- a 3-digit MNC
    // under 100 (e.g. "001") is otherwise indistinguishable from a 2-digit one.
    uint8_t mncLen() const {return static_cast<uint8_t>(getMnc().length());};

private:
    std::shared_ptr<reftools::mbsf::PlmnId> m_plmnId;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_PLMN_ID_HH_ */
