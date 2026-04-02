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
#include "ServiceScheduleDesc.hh"
#include "UserDataIngSession.hh"
#include "utilities.hh"

#include "openapi/model/TimeWindow.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSUserDataIngSession.h"

#include "ActivePeriods.hh"

using reftools::mbsf::TimeWindow;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::DistSessionState;

MBSF_NAMESPACE_START

using TimestampAndActiveFlag = ActivePeriodsBase::TimestampAndActiveFlag;
using ActPeriodsType = MBSUserDataIngSession::ActPeriodsType;

static DistSessionState dist_session_state_active;
static DistSessionState dist_session_state_inactive;
static DistSessionState dist_session_state_established;
static DistSessionState dist_session_state_no_val;

static std::optional<std::chrono::system_clock::time_point> parse_date_time(const std::string& date_time);
static fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type extract_time_windows(const ActPeriodsType &actPeriods);
static fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type extract_time_windows(ActPeriodsType &&actPeriods);
static void ensure_dist_session_state_statics();

ActivePeriods::ActivePeriods(const ActPeriodsType &act_periods, const std::shared_ptr<ActivePeriodsBase> &old_active_periods, UserDataIngSession &user_data_ing_session)
    :ActivePeriodsBase(user_data_ing_session.userDataIngSessionId())
    ,m_actPeriodsTP()
{
    convertActPeriods(act_periods, old_active_periods, user_data_ing_session);
}

ActivePeriods::ActivePeriods(ActPeriodsType &&act_periods, const std::shared_ptr<ActivePeriodsBase> &old_active_periods, UserDataIngSession &user_data_ing_session)
    :ActivePeriodsBase(user_data_ing_session.userDataIngSessionId())
    ,m_actPeriodsTP()
{
    convertActPeriods(std::move(act_periods), old_active_periods, user_data_ing_session);
}

const DistSessionState &ActivePeriods::currentState(const MbsDistSessStateType &dist_sess_state) const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    ensure_dist_session_state_statics();

    // Evaluate first window with end > now
    for (auto &tw : m_actPeriodsTP) {
        const auto& start = tw.start;
        const auto& end = tw.end;

        if (end <= now) continue; // already finished window, skip to next window

        if (start <= now) {
            // Inside current window, current state is active
            //ogs_debug("Current state is ACTIVE");
            return dist_session_state_active;
        }

        // start > now
        // not in a window, next window starts in the future
        auto pre_establish_time = start - establish_pre_start_seconds;
        if (pre_establish_time <= now) {
            // After pre_establish_time, so within the establishing period before the next window, current state is established
            //ogs_debug("Current state is ESTABLISHED");
            return dist_session_state_established;
        } else {
            // Before pre_establish_time, so within the inactive time between windows
            //ogs_debug("Current state is INACTIVE");
            return dist_session_state_inactive;
        }
    }

    // we are past all windows so the current state is inactive
    //ogs_debug("Past all windows, state is INACTIVE");
    return dist_session_state_inactive;
}

TimestampAndActiveFlag ActivePeriods::nextTransition() const
{
    std::chrono::seconds establish_pre_start_seconds{App::self().context()->actPeriodEstablishedStateDuration};
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    DistSessionState dist_session_state;

    // Evaluate first window with end > now
    for (auto &tw : m_actPeriodsTP) {
        const auto& end = tw.end;
        if (end <= now) continue; // already finished this window, go to next

        const auto& start = tw.start;
        if (start <= now) {
            // In the window, so the next state change would be to inactive at the window end time
            dist_session_state = DistSessionState::VAL_INACTIVE;
            //ogs_debug("%s", std::format("In the window, so the next state change would be to inactive at {}", end).c_str());
            return {end, dist_session_state};
        }

        // start > now
        // outside a window, next window starts in the future
        auto pre_establish_time = start - establish_pre_start_seconds;
        if (pre_establish_time <= now) {
            // Within establish_pre_start_seconds of the start time, so the next transition is active at the start time
            dist_session_state = DistSessionState::VAL_ACTIVE;
            //ogs_debug("%s", std::format("Within {0} of {1}, so the next transition is active at {1}", establish_pre_start_seconds, start).c_str());
            return {start, dist_session_state};
        } else {
            // Before establish_pre_start_seconds before the start time, so next transition is established at pre_establish_time
            dist_session_state = DistSessionState::VAL_ESTABLISHED;
            //ogs_debug("%s", std::format("Before {} of {}, so next transition is established at {}", establish_pre_start_seconds, start, pre_establish_time).c_str());
            return {pre_establish_time, dist_session_state};
        }
    }

    // All windows finished, remain inactive
    dist_session_state = DistSessionState::VAL_INACTIVE;
    return {std::nullopt, dist_session_state};
}

std::optional<std::list<std::shared_ptr<ServiceScheduleDesc> > > ActivePeriods::serviceScheduleDescriptions() const
{
    if (m_actPeriodsTP.empty()) return std::nullopt;

    std::list<std::shared_ptr<ServiceScheduleDesc> > ret_val;

    for (const auto &ver_time_win : m_actPeriodsTP) {
        ret_val.emplace_back(new ServiceScheduleDesc(m_id, ver_time_win.version, time_point_to_iso8601_utc_str(ver_time_win.start), time_point_to_iso8601_utc_str(ver_time_win.end)));
    }

    return ret_val;
}

void ActivePeriods::convertActPeriods(const ActPeriodsType& act_periods,
                                      const std::shared_ptr<ActivePeriodsBase> &old_active_periods,
                                      UserDataIngSession &user_data_ing_session)
{
    auto act_periods_list = extract_time_windows(act_periods);
    return convertActPeriods(act_periods_list, old_active_periods, user_data_ing_session);
}

void ActivePeriods::convertActPeriods(ActPeriodsType&& act_periods,
                                      const std::shared_ptr<ActivePeriodsBase> &old_active_periods,
                                      UserDataIngSession &user_data_ing_session)
{
    auto act_periods_list = extract_time_windows(std::move(act_periods));

    return convertActPeriods(act_periods_list, old_active_periods, user_data_ing_session);
}

void ActivePeriods::convertActPeriods(const fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type& act_periods_list,
                                      const std::shared_ptr<ActivePeriodsBase> &old_active_periods,
                                      UserDataIngSession &user_data_ing_session)
{
    m_actPeriodsTP.clear();
    auto old_act_periods = std::dynamic_pointer_cast<ActivePeriods>(old_active_periods);
    for (const auto &time_window : act_periods_list)
    {
        if (!time_window || !time_window.value()) continue;

        const std::shared_ptr<TimeWindow>& time_win = time_window.value();

        std::optional<std::chrono::system_clock::time_point> start_time = parse_date_time(time_win->getStartTime());
        std::optional<std::chrono::system_clock::time_point> stop_time = parse_date_time(time_win->getStopTime());

        if (!start_time || !stop_time) continue;

        ActivePeriods::VersionedTimeWindowTP tw{.start = start_time.value(), .end = stop_time.value()};
        if (old_act_periods) {
            auto it = std::find_if(old_act_periods->m_actPeriodsTP.begin(), old_act_periods->m_actPeriodsTP.end(), [&tw](const ActivePeriods::VersionedTimeWindowTP &other) { return other.start == tw.start && other.end == tw.end; });
            if (it != old_act_periods->m_actPeriodsTP.end()) {
                tw.version = it->version;
            } else {
                tw.version = user_data_ing_session.serviceScheduleDescVersion();
            }
        } else {
            tw.version = user_data_ing_session.serviceScheduleDescVersion();
        }

        m_actPeriodsTP.push_back(std::move(tw));
    }
    m_actPeriodsTP.sort([](auto const& a, auto const& b) {
        return a.start < b.start;
    });

}

static std::optional<std::chrono::system_clock::time_point> parse_date_time(const std::string& date_time) {
#if 0
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
#else
    std::chrono::system_clock::time_point result = to_time_point_iso8601(date_time);
    if (result == std::chrono::system_clock::time_point{}) return std::nullopt;
    return result;
#endif
}

// conversion function
static fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type extract_time_windows(const ActPeriodsType &actPeriods) {
    if (!actPeriods.has_value()) {
        return {}; // empty list when optional is empty
    }
    return fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type(actPeriods.value());
}

// move-aware overload to avoid copies when you can move
static fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type extract_time_windows(ActPeriodsType &&actPeriods) {
    if (!actPeriods.has_value()) {
        return {};
    }
    return fiveg_mag_reftools::remove_std_optional<ActPeriodsType>::type(std::move(*actPeriods));
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
