#ifndef _MBSF_ACTIVE_PERIODS_BASE_HH_
#define _MBSF_ACTIVE_PERIODS_BASE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Active Periods Base class
 ******************************************************************************
 * Copyright: (C)2025-2026 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 *            David Waring <david.waring2@bbc.co.uk>
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

#include <chrono>
#include <optional>
#include <utility>

#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "common.hh"


MBSF_NAMESPACE_START

class ServiceScheduleDesc;

class ActivePeriodsBase {
public:
    using DistSessionState = reftools::mbsf::DistSessionState;
    using SysTimeMS = std::chrono::system_clock::time_point;
    using TimestampAndActiveFlag = std::pair<std::optional<SysTimeMS>, DistSessionState >;
    using ActPeriodsType = reftools::mbsf::MBSUserDataIngSession::ActPeriodsType;
    using MbsDistSessStateType = reftools::mbsf::MBSDistributionSessionInfo::MbsDistSessStateType;
    using TimeRange = std::pair<std::optional<SysTimeMS>, std::optional<SysTimeMS>>;

    ActivePeriodsBase(const std::string &user_data_ing_sess_id) : m_id(user_data_ing_sess_id) {};

    virtual ~ActivePeriodsBase() = default;
    virtual const DistSessionState &currentState(const MbsDistSessStateType &dist_session_state) const = 0;
    virtual TimestampAndActiveFlag nextTransition() const = 0;
    virtual std::optional<std::list<std::shared_ptr<ServiceScheduleDesc> > > serviceScheduleDescriptions() const = 0;
    virtual TimeRange activeTimeRange() const = 0;

protected:
    std::string m_id;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_ACTIVE_PERIODS_BASE_HH_ */
