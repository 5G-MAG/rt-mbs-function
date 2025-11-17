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
#include <iostream>

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "ActivePeriodsBase.hh"

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

ActivePeriodsRepRule::ActivePeriodsRepRule(const ActPeriodsRepRuleType &act_periods_rep_rule)
    :m_repetitionRule(nullptr)
{
    if(act_periods_rep_rule.has_value()) {
        m_repetitionRule = act_periods_rep_rule.value();
    }
}

const DistSessionState &ActivePeriodsRepRule::currentState(const MbsDistSessStateType &dist_sess_state) const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    static DistSessionState dist_session_state;

    std::optional<std::chrono::system_clock::time_point> start = parse_date_time(m_repetitionRule->getStartTime());
    int32_t duration = m_repetitionRule->getDuration();
    int32_t repetition_interval = m_repetitionRule->getRepetitionInterval();
 
    std::chrono::seconds dur{duration};
    std::chrono::seconds rep_interval{repetition_interval}; 

    if(!start.has_value()) {
	    
	ogs_info("REP RULE START TIME HAS NO VAL");
         dist_session_state = DistSessionState::NO_VAL;
         return dist_session_state;
    }

    // Calculate time difference from the rule's start
    auto diff = now - start.value();

    if (diff < -establish_pre_start_seconds) {
	dist_session_state = DistSessionState::VAL_INACTIVE;
        return dist_session_state;    
    } else if (diff < std::chrono::milliseconds::zero()) {
        dist_session_state = DistSessionState::VAL_ESTABLISHED;
        return dist_session_state;
    } else {

	auto offset = std::chrono::duration_cast<std::chrono::seconds>(diff % rep_interval);
        auto active_start = now - offset;

        if (offset < dur) {
            dist_session_state = DistSessionState::VAL_ACTIVE;
            return dist_session_state;
        } else if (offset < rep_interval - establish_pre_start_seconds) {
            dist_session_state = DistSessionState::VAL_INACTIVE;
            return dist_session_state; 
        } else {
            dist_session_state = DistSessionState::VAL_ESTABLISHED;
            return dist_session_state;
        }
	dist_session_state = DistSessionState::VAL_INACTIVE;
        return dist_session_state;


    }

    dist_session_state = DistSessionState::VAL_INACTIVE;
    return dist_session_state;
}


TimestampAndActiveFlag ActivePeriodsRepRule::nextTransition () const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    static DistSessionState dist_session_state;

    std::optional<std::chrono::system_clock::time_point> start = parse_date_time(m_repetitionRule->getStartTime());
    int32_t duration = m_repetitionRule->getDuration();
    int32_t repetition_interval = m_repetitionRule->getRepetitionInterval();

    std::chrono::seconds dur{duration};
    std::chrono::seconds rep_interval{repetition_interval};

    if(!start.has_value()) {
         dist_session_state = DistSessionState::NO_VAL;
	 return {std::nullopt, dist_session_state};
    }
    
    // Calculate time difference from the rule's start
    auto diff = now - start.value();

    if (diff < -establish_pre_start_seconds) {
        dist_session_state = DistSessionState::VAL_ESTABLISHED;
        return {(start.value() - establish_pre_start_seconds), dist_session_state} ;
    } else if (diff < std::chrono::seconds::zero()) {
        dist_session_state = DistSessionState::VAL_ACTIVE;
        return {start.value(), dist_session_state};
    } else {

        auto offset = std::chrono::duration_cast<std::chrono::seconds>(diff % rep_interval);

        auto active_start = now - offset;

        if (offset < dur) {
            dist_session_state = DistSessionState::VAL_INACTIVE;
            return {(active_start + dur), dist_session_state};
        } else if (offset < (rep_interval - establish_pre_start_seconds)) {
	    dist_session_state = DistSessionState::VAL_ESTABLISHED;
            return {active_start + rep_interval - establish_pre_start_seconds, dist_session_state};
        } else {
            dist_session_state = DistSessionState::VAL_ACTIVE;
            return {active_start + rep_interval, dist_session_state};
         }
    }

    dist_session_state = DistSessionState::VAL_INACTIVE;
    return {std::nullopt, dist_session_state};
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
	ogs_info("PARSE TIME SS FAIL");
        return std::nullopt;
    }

    std::time_t time = std::mktime(&tm);
    if (time == -1) {

	ogs_info("PARSE TIME MKTIME RETURNS -1");
	return std::nullopt;
    }

    auto tp = std::chrono::system_clock::from_time_t(time);
    tp += std::chrono::milliseconds(milliseconds);
    return tp;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
