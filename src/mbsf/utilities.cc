/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Common utility functions
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
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
#include <chrono>
#include <format>
#include <string>
#include <sstream>

#include "common.hh"

#include "utilities.hh"

MBSF_NAMESPACE_START

std::string trim_slashes(const std::string &path)
{
    size_t start = path.starts_with('/') ? 1 : 0;
    size_t end = path.ends_with('/') ? path.size() - 1 : path.size();

    return path.substr(start, end - start);
}

std::string time_point_to_http_datetime_str(const std::chrono::system_clock::time_point &datetime)
{
    return std::format("%b, %d %b %Y %T GMT", datetime);
}

std::string time_point_to_iso8601_utc_str(const std::chrono::system_clock::time_point &datetime)
{
    std::ostringstream oss;
    auto datetime_us = std::chrono::time_point_cast<std::chrono::microseconds>(datetime);
    oss.imbue(std::locale("C"));
    oss << std::format("{0:%F}T{0:%T}Z", datetime_us);
    return oss.str();
}

std::chrono::system_clock::time_point to_time_point_iso8601(const std::string &datetime) 
{
    using sys_us = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;
    sys_us tp_us;

    std::istringstream iss(datetime);
    iss >> std::chrono::parse("%Y-%m-%dT%H:%M:%S", tp_us);

    if (iss.fail()) {
        return std::chrono::system_clock::time_point{}; // epoch indicates failure
    }

    // Accept optional 'Z' (or 'z') but do not require it.
    char c = 0;
    if (iss >> c) {
        if (c != 'Z' && c != 'z') {
            iss.unget(); // not a Z, put it back
        }
    } else {
        // clear eof state so ws/eof check below works
        iss.clear();
    }

    // Allow only trailing whitespace after optional Z
    iss >> std::ws;
    if (!iss.eof()) {
        return std::chrono::system_clock::time_point{}; // leftover characters -> failure
    }

    // Convert microsecond-precision sys_time to system_clock::time_point
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_us);
}

std::string time_t_to_str(time_t time)
{
    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    if (localtime_s(&tm, &time) != 0) return std::string();
#else
    if (localtime_r(&time, &tm) == nullptr) return std::string();
#endif

    char buf[64];
    if (std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y\n", &tm) == 0) {
        return std::string();
    }
    return std::string(buf);
}





MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
