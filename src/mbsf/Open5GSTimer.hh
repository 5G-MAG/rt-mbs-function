#ifndef _MBSF_OPEN5GS_TIMER_HH_
#define _MBSF_OPEN5GS_TIMER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Open5GS ogs_timer_t wrapper
 ******************************************************************************
 * Copyright: (C)2024-2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *            Dev Audsin <dev.audsin@bbc.co.uk>
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
#include "ogs-core.h"

#include <memory>

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSTimer {
public:
    Open5GSTimer(ogs_timer_t *timer) :m_ogsTimer(timer) {};
    Open5GSTimer() = delete;
    Open5GSTimer(Open5GSTimer &&other) = delete;
    Open5GSTimer(const Open5GSTimer &other) = delete;
    Open5GSTimer &operator=(Open5GSTimer &&other) = delete;
    Open5GSTimer &operator=(const Open5GSTimer &other) = delete;
    virtual ~Open5GSTimer() {if (!m_ogsTimer) return; ogs_timer_stop(m_ogsTimer); ogs_timer_delete(m_ogsTimer);};

    ogs_timer_t *ogsTimer() { return m_ogsTimer; };
    const ogs_timer_t *ogsTimer() const { return m_ogsTimer; };

    void start(int milliseconds) { if (!m_ogsTimer) return; ogs_timer_start(m_ogsTimer, ogs_time_from_msec(milliseconds)); };

private:
    ogs_timer_t *m_ogsTimer;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OPEN5GS_TIMER_HH_ */
