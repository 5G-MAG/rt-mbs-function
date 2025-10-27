/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Active Periods
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

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "ActivePeriodsBase.hh"

#include "openapi/model/TimeWindow.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSUserDataIngSession.h"

namespace reftools::mbsf {
    class TimeWindow;
    class MBSUserDataIngSession;
    class DistSessionState;
}

using reftools::mbsf::TimeWindow;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::DistSessionState;

#include "ActivePeriods.hh"


MBSF_NAMESPACE_START

using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;   
using ActPeriodsType = MBSUserDataIngSession::ActPeriodsType;

static std::optional<std::chrono::system_clock::time_point> parse_date_time(const std::string& date_time);
static std::list<std::optional<std::shared_ptr<TimeWindow>>> extract_time_windows(const ActPeriodsType &actPeriods);
static std::list<std::optional<std::shared_ptr<TimeWindow>>> extract_time_windows(const ActPeriodsType &&actPeriods);
static void convert_act_periods(const ActPeriodsType& act_periods, std::list<ActivePeriods::TimeWindowTP> &act_periods_tp);

ActivePeriods::ActivePeriods(const ActPeriodsType &act_periods)
{
    convert_act_periods(act_periods, m_actPeriodsTP);

}

const DistSessionState &ActivePeriods::currentState(const MbsDistSessStateType &dist_sess_state) const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    static DistSessionState dist_session_state;

    // Evaluate first window with end > now
    for (auto &tw : m_actPeriodsTP) {
        const auto& start = tw.first;
        const auto& end = tw.second;

        if (end <= now) continue; // already finished window

        if (start <= now) {
            dist_session_state = DistSessionState::VAL_ACTIVE;
	    return dist_session_state;
        }

        // start > now
        auto pre_establish_time = start - establish_pre_start_seconds;
        if (pre_establish_time <= now) {
            dist_session_state = DistSessionState::VAL_ESTABLISHED;
	    return dist_session_state;
        } else {
	    
            dist_session_state = DistSessionState::VAL_INACTIVE;
	    return dist_session_state;
        }


    }
    dist_session_state = DistSessionState::VAL_INACTIVE;
    return dist_session_state;
}

TimestampAndActiveFlag ActivePeriods::nextTransition () const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    static DistSessionState dist_session_state;
    
    // Evaluate first window with end > now
    for (auto &tw : m_actPeriodsTP) {
        const auto& start = tw.first;
        const auto& end = tw.second;

        if (end <= now) continue; // already finished window
	
        if (start <= now) {
	    dist_session_state = DistSessionState::VAL_INACTIVE;
            return {end, dist_session_state};
        }

        // start > now
        auto pre_establish_time = start - establish_pre_start_seconds;
        if (pre_establish_time <= now) {
	    
	    dist_session_state = DistSessionState::VAL_ACTIVE;
	    return {start, dist_session_state};
        } else {

	    dist_session_state = DistSessionState::VAL_ESTABLISHED;
	    return {pre_establish_time, dist_session_state};
        } 

    }
    dist_session_state = DistSessionState::VAL_INACTIVE;
    return {std::nullopt, dist_session_state};
}


static void convert_act_periods(const ActPeriodsType& act_periods, std::list<ActivePeriods::TimeWindowTP> &act_periods_tp)
{
    static std::list<std::optional<std::shared_ptr<TimeWindow>>> act_periods_list = extract_time_windows(act_periods);

    act_periods_tp.clear();
    for (const auto &time_window : act_periods_list)
    {
        if (!time_window.has_value())
        {
            continue;
        }

        const std::shared_ptr<TimeWindow>& time_win = *time_window;
        if (!time_win)
        {
            continue;
        }

        std::optional<std::chrono::system_clock::time_point> start_time = parse_date_time(time_win->getStartTime());
        std::optional<std::chrono::system_clock::time_point> stop_time = parse_date_time(time_win->getStopTime());
        
	if(!start_time.has_value() || !stop_time.has_value()) continue;

	if (start_time.has_value() && stop_time.has_value()) {
            act_periods_tp.emplace_back(std::make_pair(start_time.value(), stop_time.value()));
        }
    }
    act_periods_tp.sort([](auto const& a, auto const& b) {
        return a.first < b.first;
    });

}

static std::optional<std::chrono::system_clock::time_point> parse_date_time(const std::string& date_time) {
    std::string dt = date_time;

    // Remove trailing 'Z' if present
    if (!dt.empty() && dt.back() == 'Z') {
        dt.pop_back();
    }

    // Split into main time and milliseconds
    std::string main_time;
    int milliseconds = 0;
    size_t dot_pos = dt.find('.');
    if (dot_pos != std::string::npos) {
        main_time = dt.substr(0, dot_pos);
        std::string msec_str = dt.substr(dot_pos + 1);
        if (msec_str.size() > 3) msec_str = msec_str.substr(0, 3); // trim to 3 digits
        while (msec_str.size() < 3) msec_str += '0'; // pad if needed
        milliseconds = std::stoi(msec_str);
    } else {
        main_time = dt;
    }

    // Parse main time
    std::tm tm = {};
    std::istringstream ss(main_time);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) return std::nullopt;

    std::time_t time = std::mktime(&tm);
    if (time == -1) return std::nullopt;

    auto tp = std::chrono::system_clock::from_time_t(time);
    tp += std::chrono::milliseconds(milliseconds);
    return tp;
}

// conversion function
static std::list<std::optional<std::shared_ptr<TimeWindow>>> extract_time_windows(const ActPeriodsType &actPeriods) {
    if (!actPeriods.has_value()) {
        return {}; // empty list when optional is empty
    }
    const std::list<std::optional<std::shared_ptr<TimeWindow>>, fiveg_mag_reftools::OgsAllocator<std::optional<std::shared_ptr<TimeWindow>>>> &src = *actPeriods;
    return std::list<std::optional<std::shared_ptr<TimeWindow>>>(src.begin(), src.end());
}

// move-aware overload to avoid copies when you can move
static std::list<std::optional<std::shared_ptr<TimeWindow>>> extract_time_windows(const ActPeriodsType &&actPeriods) {
    if (!actPeriods.has_value()) {
        return {};
    }
    std::list<std::optional<std::shared_ptr<TimeWindow>>, fiveg_mag_reftools::OgsAllocator<std::optional<std::shared_ptr<TimeWindow>>>> src = std::move(*actPeriods);
    return std::list<std::optional<std::shared_ptr<TimeWindow>>>(std::make_move_iterator(src.begin()),
                                  std::make_move_iterator(src.end()));
}



MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
