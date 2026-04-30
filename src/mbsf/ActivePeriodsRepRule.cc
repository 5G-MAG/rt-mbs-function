/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Active Periods Rep Rule
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
#include <list>
#include <memory>
#include <optional>
#include <iomanip>
#include <iostream>

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "ActivePeriodsBase.hh"
#include "ServiceScheduleDesc.hh"

#include "openapi/model/RepetitionRule.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSUserDataIngSession.h"

namespace reftools::mbsf {
    class RepetitionRule;
    class MBSUserDataIngSession;
    class DistSessionState;
}

using reftools::mbsf::RepetitionRule;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::DistSessionState;

#include "ActivePeriodsRepRule.hh"

MBSF_NAMESPACE_START

using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;
using ActPeriodsRepRuleType = MBSUserDataIngSession::ActPeriodsRepRuleType;

static std::optional<std::chrono::system_clock::time_point> parse_date_time(const std::string& date_time);

static DistSessionState dist_session_state_active;
static DistSessionState dist_session_state_inactive;
static DistSessionState dist_session_state_established;
static DistSessionState dist_session_state_no_val;

static void ensure_dist_session_state_statics();

ActivePeriodsRepRule::ActivePeriodsRepRule(const ActPeriodsRepRuleType &act_periods_rep_rule, const std::shared_ptr<ActivePeriodsBase> &old_active_periods, UserDataIngSession &user_data_ingest_session)
    :ActivePeriodsBase(user_data_ingest_session.userDataIngSessionId())
    ,m_repetitionRule(nullptr)
{
    if (act_periods_rep_rule.has_value()) {
        uint32_t version;
        auto old_act_periods_rep_rule = std::dynamic_pointer_cast<ActivePeriodsRepRule>(old_active_periods);
        if (old_act_periods_rep_rule && *old_act_periods_rep_rule->m_repetitionRule->second == *act_periods_rep_rule.value()) {
            version = old_act_periods_rep_rule->m_repetitionRule->first;
        } else {
            version = user_data_ingest_session.serviceScheduleDescVersion();
        }
        m_repetitionRule.reset(new VersionedRepetitionRule{version, act_periods_rep_rule.value()});
    }
}

const DistSessionState &ActivePeriodsRepRule::currentState(const MbsDistSessStateType &dist_sess_state) const
{
    ensure_dist_session_state_statics();

    if (!m_repetitionRule) return dist_session_state_no_val;

    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    std::optional<std::chrono::system_clock::time_point> start = parse_date_time(m_repetitionRule->second->getStartTime());
    if (!start.has_value()) {
        ogs_debug("REP RULE START TIME HAS NO VAL");
        return dist_session_state_no_val;
    }

    int32_t duration = m_repetitionRule->second->getDuration();
    int32_t repetition_interval = m_repetitionRule->second->getRepetitionInterval();

    std::chrono::seconds dur{duration};
    std::chrono::seconds rep_interval{repetition_interval};

    // Calculate time difference from the rule's start
    auto diff = now - start.value();

    if (diff < -establish_pre_start_seconds) {
        // rule has not started, and is more than establish_pre_start_seconds before the start
        return dist_session_state_inactive;
    } else if (diff < std::chrono::system_clock::duration::zero()) {
        // rule has not started, but is within establish_pre_start_seconds of the start
        return dist_session_state_established;
    } else {
        // rule has started, so get our offset into the current rule window
        auto offset = std::chrono::duration_cast<std::chrono::seconds>(diff % rep_interval);

        if (offset < dur) {
            // within the first dur seconds of the current window, so the window is active
            return dist_session_state_active;
        } else if (offset < rep_interval - establish_pre_start_seconds) {
            // greater than dur seconds but earlier than establish_pre_start_seconds before the next window, so inactive
            return dist_session_state_inactive;
        } else {
            // within establish_pre_start_seconds of the next window, so we need to establish the session
            return dist_session_state_established;
        }
    }

    // Shouldn't reach here, but just incase
    return dist_session_state_inactive;
}

TimestampAndActiveFlag ActivePeriodsRepRule::nextTransition () const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    ensure_dist_session_state_statics();

    std::optional<std::chrono::system_clock::time_point> start = parse_date_time(m_repetitionRule->second->getStartTime());
    if (!start.has_value()) {
         return {std::nullopt, dist_session_state_no_val};
    }

    int32_t duration = m_repetitionRule->second->getDuration();
    int32_t repetition_interval = m_repetitionRule->second->getRepetitionInterval();

    std::chrono::seconds dur{duration};
    std::chrono::seconds rep_interval{repetition_interval};

    // Calculate time difference from the rule's start
    auto diff = now - start.value();

    if (diff < -establish_pre_start_seconds) {
        // rule has not started and more than establish_pre_start_seconds before the rule start
        return {(start.value() - establish_pre_start_seconds), dist_session_state_established} ;
    } else if (diff < std::chrono::system_clock::duration::zero()) {
        // rule has not started but is within establish_pre_start_seconds if the start
        return {start.value(), dist_session_state_active};
    } else {

        auto offset = std::chrono::duration_cast<std::chrono::seconds>(diff % rep_interval);

        auto active_start = start.value() + (rep_interval * (diff / rep_interval));

        if (offset < dur) {
            return {(active_start + dur), dist_session_state_inactive};
        } else if (offset < (rep_interval - establish_pre_start_seconds)) {
            return {active_start + rep_interval - establish_pre_start_seconds, dist_session_state_established};
        } else {
            return {active_start + rep_interval, dist_session_state_active};
        }
    }

    return {std::nullopt, dist_session_state_inactive};
}

ActivePeriodsRepRule::TimeRange ActivePeriodsRepRule::activeTimeRange() const
{
    if (!m_repetitionRule) return TimeRange(std::nullopt, std::nullopt);
    return TimeRange(parse_date_time(m_repetitionRule->second->getStartTime()), std::nullopt);
}

std::optional<std::list<std::shared_ptr<ServiceScheduleDesc> > > ActivePeriodsRepRule::serviceScheduleDescriptions() const
{
    if (!m_repetitionRule) return std::nullopt;

    std::shared_ptr<ServiceScheduleDesc> ssd(new ServiceScheduleDesc(m_id, m_repetitionRule->first, m_repetitionRule->second));
    return std::list<std::shared_ptr<ServiceScheduleDesc> >{ssd};
}

static std::optional<std::chrono::system_clock::time_point> parse_date_time(const std::string& date_time) {
    std::string dt = date_time;

    if (!dt.empty() && dt.back() == 'Z') {
        dt.pop_back();
    }

    std::string main_time;
    int milliseconds = 0;
    size_t dot_pos = dt.find('.');
    if (dot_pos != std::string::npos) {
        main_time = dt.substr(0, dot_pos);
        std::string msec_str = dt.substr(dot_pos + 1);
        if (msec_str.size() > 3) msec_str = msec_str.substr(0, 3);
        while (msec_str.size() < 3) msec_str += '0';
        milliseconds = std::stoi(msec_str);
    } else {
        main_time = dt;
    }

    std::tm tm = {};
    std::istringstream ss(main_time);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        return std::nullopt;
    }

    std::time_t time = std::mktime(&tm);
    if (time == -1) {

        return std::nullopt;
    }

    auto tp = std::chrono::system_clock::from_time_t(time);
    tp += std::chrono::milliseconds(milliseconds);
    return tp;
}

static void ensure_dist_session_state_statics()
{
    if (dist_session_state_active.getValue() != DistSessionState::VAL_ACTIVE) {
        dist_session_state_active = DistSessionState::VAL_ACTIVE;
        dist_session_state_inactive = DistSessionState::VAL_INACTIVE;
        dist_session_state_established = DistSessionState::VAL_ESTABLISHED;
    }
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
