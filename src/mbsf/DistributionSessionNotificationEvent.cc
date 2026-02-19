/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Distribution Session Notification Event
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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

#include "common.hh"
#include "DistributionSessionInfoSubscription.hh"
#include "LocalEvents.hh"
#include "Open5GSEvent.hh"

#include "DistributionSessionNotificationEvent.hh"

MBSF_NAMESPACE_START

namespace {
  struct EventData {
    const DistributionSessionInfoSubscription &dist_session;
  };
}

DistributionSessionNotificationEvent::DistributionSessionNotificationEvent(Open5GSEvent &event)
    :Open5GSEvent(event.ogsEvent())
{
}

DistributionSessionNotificationEvent::DistributionSessionNotificationEvent(const DistributionSessionInfoSubscription &dist_session)
    :Open5GSEvent(new ogs_event_t)
{
    ogs_event_t *evt = ogsEvent();
    evt->id = LocalEvents::SEND_NOTIFICATION;
    EventData *evt_data = new EventData{dist_session};
    evt->sbi.data = reinterpret_cast<void*>(evt_data);
}

const DistributionSessionInfoSubscription &DistributionSessionNotificationEvent::distributionSessionInfoSubscription() const
{
    EventData *evt_data = reinterpret_cast<EventData*>(sbiData());
    return evt_data->dist_session;
}

void DistributionSessionNotificationEvent::releaseEventData()
{
    EventData *evt_data = reinterpret_cast<EventData*>(sbiData());
    delete evt_data;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
