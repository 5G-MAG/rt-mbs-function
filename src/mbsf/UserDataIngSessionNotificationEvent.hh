#ifndef _MBSF_USER_DATA_ING_SESSION_NOTIFICATION_EVENT_HH_
#define _MBSF_USER_DATA_ING_SESSION_NOTIFICATION_EVENT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: User Data Ing Session Notification Event
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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
#include "Open5GSEvent.hh"

MBSF_NAMESPACE_START

class UserDataIngSession;

class UserDataIngSessionNotificationEvent : public Open5GSEvent {
public:
    /* Constructors and Destructor */
    UserDataIngSessionNotificationEvent(Open5GSEvent &event);
    UserDataIngSessionNotificationEvent(const UserDataIngSession &session);
    UserDataIngSessionNotificationEvent() = delete;
    UserDataIngSessionNotificationEvent(UserDataIngSessionNotificationEvent &&other) = delete;
    UserDataIngSessionNotificationEvent(const UserDataIngSessionNotificationEvent &other) = delete;

    virtual ~UserDataIngSessionNotificationEvent() {};

    /* operators */
    UserDataIngSessionNotificationEvent &operator=(UserDataIngSessionNotificationEvent &&other) = delete;
    UserDataIngSessionNotificationEvent &operator=(const UserDataIngSessionNotificationEvent &other) = delete;

    const UserDataIngSession &userDataIngSession() const;
    void releaseEventData();
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_DATA_ING_SESSION_NOTIFICATION_EVENT_HH_ */
