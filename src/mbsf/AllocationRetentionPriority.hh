#ifndef _MBSF_ARP_HH_
#define _MBSF_ARP_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS ARP class
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
#include "openapi/model/Arp.h"
#include "openapi/model/PreemptionCapability.h"
#include "openapi/model/PreemptionVulnerability.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class Arp;
}

MBSF_NAMESPACE_START

class AllocationRetentionPriority {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    AllocationRetentionPriority(fiveg_mag_reftools::CJson &json, bool as_request);
    AllocationRetentionPriority(const std::shared_ptr<reftools::mbsf::Arp> &arp);
    AllocationRetentionPriority() = delete;
    AllocationRetentionPriority(AllocationRetentionPriority &&other) = delete;
    AllocationRetentionPriority(const AllocationRetentionPriority &other) = delete;
    AllocationRetentionPriority &operator=(AllocationRetentionPriority &&other) = delete;
    AllocationRetentionPriority &operator=(const AllocationRetentionPriority &other) = delete;

    virtual ~AllocationRetentionPriority();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::Arp> &getArp() const {return m_arp;};
    const int32_t getPriorityLevel() const {return m_arp->getPriorityLevel();};
    const std::shared_ptr<reftools::mbsf::PreemptionCapability> &getPreemptionCapability() const { return m_arp->getPreemptCap();};
    const std::shared_ptr<reftools::mbsf::PreemptionVulnerability> &getPreemptionVulnerability() const {
        return m_arp->getPreemptVuln();
    };

    mb_smf_sc_preemption_capability_e preemptionCapabilityLookup();
    mb_smf_sc_preemption_vulnerability_e preemptionVulnerabilityLookup();
    mb_smf_sc_arp_t *populateArp();

private:
    std::shared_ptr<reftools::mbsf::Arp> m_arp;
    static const std::unordered_map<std::string, mb_smf_sc_preemption_capability_e> preemptionCap;
    static const std::unordered_map<std::string, mb_smf_sc_preemption_vulnerability_e> preemptionVuln;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_SERVICE_INFO_HH_ */
