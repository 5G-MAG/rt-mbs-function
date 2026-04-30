#ifndef _MBSF_ACTIVE_PERIODS_HH_
#define _MBSF_ACTIVE_PERIODS_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Active Periods class
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

#include <vector>
#include <chrono>
#include <optional>

#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/TimeWindow.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "ActivePeriodsBase.hh"
#include "common.hh"

namespace reftools::mbsf {
    class TimeWindow;
    class MBSUserDataIngSession;
    class MBSDistributionSessionInfo;
    class DistSessionState;
}

MBSF_NAMESPACE_START

class ServiceScheduleDesc;
class UserDataIngSession;

class ActivePeriods: public ActivePeriodsBase {
public:

    using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;
    using DistSessionState = ActivePeriodsBase::DistSessionState;
    struct VersionedTimeWindowTP {
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
        int32_t version;
    };
    using ActPeriodsType = reftools::mbsf::MBSUserDataIngSession::ActPeriodsType;
    using MbsDistSessStateType = reftools::mbsf::MBSDistributionSessionInfo::MbsDistSessStateType;
    using TimeRange = ActivePeriodsBase::TimeRange;

    ActivePeriods(const ActPeriodsType &act_periods, const std::shared_ptr<ActivePeriodsBase> &old_active_periods, UserDataIngSession &user_data_ing_session);
    ActivePeriods(ActPeriodsType &&act_periods, const std::shared_ptr<ActivePeriodsBase> &old_active_periods, UserDataIngSession &user_data_ing_session);

    virtual ~ActivePeriods() {};

    virtual const DistSessionState &currentState(const MbsDistSessStateType &dist_session_state) const;
    virtual TimestampAndActiveFlag nextTransition() const;
    virtual std::optional<std::list<std::shared_ptr<ServiceScheduleDesc> > > serviceScheduleDescriptions() const;
    virtual TimeRange activeTimeRange() const;

private:
    void convertActPeriods(const ActPeriodsType& act_periods, const std::shared_ptr<ActivePeriodsBase> &old_active_periods,
                           UserDataIngSession &user_data_ing_session);
    void convertActPeriods(ActPeriodsType&& act_periods, const std::shared_ptr<ActivePeriodsBase> &old_active_periods,
                           UserDataIngSession &user_data_ing_session);
    void convertActPeriods(const fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type& act_periods_list,
                           const std::shared_ptr<ActivePeriodsBase> &old_active_periods,
                           UserDataIngSession &user_data_ing_session);

    std::list<VersionedTimeWindowTP> m_actPeriodsTP;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_ACTIVE_PERIODS_HH_ */
