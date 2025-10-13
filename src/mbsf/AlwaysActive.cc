/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Always Active
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
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

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <stdexcept>
#include <string>

#include "common.hh"
#include "App.hh"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/TimeWindow.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "ActivePeriodsBase.hh"

#include "AlwaysActive.hh"

namespace reftools::mbsf {
    class TimeWindow;
    class MBSUserDataIngSession;
    class DistSessionState;
}

using reftools::mbsf::TimeWindow;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::DistSessionState;

MBSF_NAMESPACE_START

using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;
using DistSessionState = ActivePeriodsBase::DistSessionState;
using ActPeriodsType = reftools::mbsf::MBSUserDataIngSession::ActPeriodsType;
using MbsDistSessStateType = reftools::mbsf::MBSDistributionSessionInfo::MbsDistSessStateType;

using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;   

const DistSessionState &AlwaysActive::currentState(const MbsDistSessStateType &dist_session_state) const
{
    static DistSessionState active;
    active = DistSessionState::VAL_ACTIVE;
    return dist_session_state?*dist_session_state.value():active;
}

TimestampAndActiveFlag AlwaysActive::nextTransition() const {
    static const DistSessionState empty;	
    return {std::nullopt, empty};
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
