/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: timer duration unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

/* Covers timerDurationMs(), which turns the time of the next active periods transition into the duration the
 * Open5GS timer is started with. The timer asserts on a duration of zero and takes whole milliseconds, so the
 * duration is at least one and is rounded up, never down: a timer set a fraction of a millisecond short fires
 * before the transition, the handler finds the same transition still ahead, and the zero duration aborted the MBSF.
 */

#include "Open5GSTimer.hh"

#include <chrono>
#include <cstdio>

using namespace com::fiveg_mag::ref_tools::mbsf;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::system_clock;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

int main()
{
    const auto now = system_clock::now();
    check(timerDurationMs(now + milliseconds(1500), now) == 1500, "whole milliseconds are kept");
    check(timerDurationMs(now + microseconds(1500500), now) == 1501, "a fraction of a millisecond rounds up");
    check(timerDurationMs(now + microseconds(1), now) == 1, "a microsecond ahead waits one millisecond");
    check(timerDurationMs(now + microseconds(999), now) == 1, "under a millisecond ahead waits one millisecond");
    check(timerDurationMs(now, now) == 1, "a transition due now is never zero");
    check(timerDurationMs(now - milliseconds(40), now) == 1, "a transition already past is never zero");
    check(timerDurationMs(now - microseconds(1), now) == 1, "a transition a microsecond past is never zero");
    std::printf("test_timer_duration: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
