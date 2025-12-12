#ifndef _MBSF_DISTRIBUTION_SESSION_INFO_HH_
#define _MBSF_DISTRIBUTION_SESSION_INFO_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Distribution Session Info class
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
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/DistSession.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class MBSDistributionSessionInfo;
    class DistSessionState;
}

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistributionMethod;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::DistSession;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::Ssm;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::PacketDistrMethInfo;
using reftools::mbsf::PktDistributionOperatingMode;
using reftools::mbsf::PktIngestMethod;
using reftools::mbsf::MbStfIngestAddr;

MBSF_NAMESPACE_START

class MBSMFMBSSession;
class ServiceInfo;

class DistributionSessionInfo {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    DistributionSessionInfo(CJson &json, bool as_request);
    DistributionSessionInfo(const std::shared_ptr<MBSDistributionSessionInfo> &mbs_distribution_session_info);
    DistributionSessionInfo() = delete;
    DistributionSessionInfo(DistributionSessionInfo &&other) = delete;
    DistributionSessionInfo(const DistributionSessionInfo &other) = delete;
    DistributionSessionInfo &operator=(DistributionSessionInfo &&other) = delete;
    DistributionSessionInfo &operator=(const DistributionSessionInfo &other) = delete;

    virtual ~DistributionSessionInfo();

    CJson json(bool as_request) const;

    const std::shared_ptr<MBSDistributionSessionInfo> &getMBSDistributionSessionInfo() const {return m_mbsDistributionSessionInfo;};
    std::shared_ptr<MBSDistributionSessionInfo> &updateMBSDistributionSessionInfo(std::shared_ptr<MBSDistributionSessionInfo> new_mbs_dist_session_infos);
    const std::shared_ptr< DistributionMethod > &getDistributionMethod() const { return m_mbsDistributionSessionInfo->getDistrMethod();}; 
      
private:
    std::shared_ptr<MBSDistributionSessionInfo> m_mbsDistributionSessionInfo;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_DISTRIBUTION_SESSION_HH_ */
