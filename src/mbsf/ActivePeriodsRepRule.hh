#ifndef _MBSF_ACTIVE_PERIODS_REP_RULE_HH_
#define _MBSF_ACTIVE_PERIODS_REP_RULE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Active Periods Rep Rule class
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
#include <list>
#include <memory>
#include <optional>

#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/RepetitionRule.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "ActivePeriodsBase.hh"
#include "common.hh"

namespace reftools::mbsf {
    class RepetitionRule;
    class MBSUserDataIngSession;
    class MBSDistributionSessionInfo;
    class DistSessionState;
}

MBSF_NAMESPACE_START

class ServiceScheduleDesc;

class ActivePeriodsRepRule: public ActivePeriodsBase {
public:

    using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;
    using DistSessionState = ActivePeriodsBase::DistSessionState;
    using ActPeriodsRepRuleType = reftools::mbsf::MBSUserDataIngSession::ActPeriodsRepRuleType;
    using MbsDistSessStateType = reftools::mbsf::MBSDistributionSessionInfo::MbsDistSessStateType;
    using RepetitionRule = reftools::mbsf::RepetitionRule;
    using VersionedRepetitionRule = std::pair<int32_t, std::shared_ptr<RepetitionRule> >;
    using TimeRange = ActivePeriodsBase::TimeRange;

    ActivePeriodsRepRule(const ActPeriodsRepRuleType &act_periods_rep_rule, const std::shared_ptr<ActivePeriodsBase> &old_active_periods, UserDataIngSession &user_data_ingest_session);

    virtual ~ActivePeriodsRepRule() {};
    virtual const DistSessionState &currentState(const MbsDistSessStateType &dist_session_state) const;
    virtual TimestampAndActiveFlag nextTransition() const;
    virtual std::optional<std::list<std::shared_ptr<ServiceScheduleDesc> > > serviceScheduleDescriptions() const;
    virtual TimeRange activeTimeRange() const;

private:
    std::shared_ptr<VersionedRepetitionRule> m_repetitionRule;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_ACTIVE_PERIODS_REP_RULE_HH_ */
