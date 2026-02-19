/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: User Data Ing Session Notification Event
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
//#include "UserDataIngSession.hh"
#include "LocalEvents.hh"
#include "Open5GSEvent.hh"

#include "UserDataIngSessionNotificationEvent.hh"

MBSF_NAMESPACE_START

namespace {
  struct EventData {
    const UserDataIngSession &user_data_ing_session;
  };
}

UserDataIngSessionNotificationEvent::UserDataIngSessionNotificationEvent(Open5GSEvent &event)
    :Open5GSEvent(event.ogsEvent())
{
}

UserDataIngSessionNotificationEvent::UserDataIngSessionNotificationEvent(const UserDataIngSession &session)
    :Open5GSEvent(new ogs_event_t)
{
    ogs_event_t *evt = ogsEvent();
    evt->id = LocalEvents::SEND_USER_DATA_ING_SESS_NOTIFICATION;
    EventData *evt_data = new EventData{session};
    evt->sbi.data = reinterpret_cast<void*>(evt_data);
}

const UserDataIngSession &UserDataIngSessionNotificationEvent::userDataIngSession() const
{
    EventData *evt_data = reinterpret_cast<EventData*>(sbiData());
    return evt_data->user_data_ing_session;
}

void UserDataIngSessionNotificationEvent::releaseEventData()
{
    EventData *evt_data = reinterpret_cast<EventData*>(sbiData());
    delete evt_data;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
