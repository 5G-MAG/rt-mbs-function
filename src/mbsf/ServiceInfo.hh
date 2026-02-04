#ifndef _MBSF_SERVICE_INFO_HH_
#define _MBSF_SERVICE_INFO_HH_
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

#include "ogs-core.h"

#include "mb-smf-service-consumer.h"

#include <memory>
#include <optional>
#include <string>

#include "common.hh"
#include "openapi/model/MbsServiceInfo.h"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class ServiceInfo {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    ServiceInfo(fiveg_mag_reftools::CJson &json, bool as_request);
    ServiceInfo(const std::shared_ptr<reftools::mbsf::MbsServiceInfo> &mbs_service_info);
    ServiceInfo() = delete;
    ServiceInfo(ServiceInfo &&other) = delete;
    ServiceInfo(const ServiceInfo &other) = delete;
    ServiceInfo &operator=(ServiceInfo &&other) = delete;
    ServiceInfo &operator=(const ServiceInfo &other) = delete;

    virtual ~ServiceInfo();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::MbsServiceInfo> &getMbsServiceInfo() const {return m_mbsServiceInfo;};
    const reftools::mbsf::MbsServiceInfo::MbsMediaCompsType &getMediaComps() const {return m_mbsServiceInfo->getMbsMediaComps();};
    const std::optional<std::shared_ptr<reftools::mbsf::ReservPriority> > &getSdfResPrio() const {
        return m_mbsServiceInfo->getMbsSdfResPrio();
    };
    const std::optional<std::string> &getAfAppId() const {return m_mbsServiceInfo->getAfAppId();};
    const std::optional<std::string> &getMbsSessionAmbr() const {return m_mbsServiceInfo->getMbsSessionAmbr();};
    ogs_hash_t *populateMediaComps();

    mb_smf_sc_mbs_service_info_t *populateServiceInfo();
    uint64_t *ambr();

private:
    std::shared_ptr<reftools::mbsf::MbsServiceInfo> m_mbsServiceInfo;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_SERVICE_INFO_HH_ */
