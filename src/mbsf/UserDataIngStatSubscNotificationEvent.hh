#ifndef _MBSF_USER_DATA_ING_STAT_SUBSC_NOTIFICATION_EVENT_HH_
#define _MBSF_USER_DATA_ING_STAT_SUBSC_NOTIFICATION_EVENT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: User Data Ing Status Subscription Notification Event
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
#include "common.hh"
#include "Open5GSEvent.hh"

MBSF_NAMESPACE_START

class DistributionSessionInfoSubscription;
class UserDataIngStatSubsc;

class UserDataIngStatSubscNotificationEvent: public Open5GSEvent {
public:
    /* Constructors and Destructor */
    UserDataIngStatSubscNotificationEvent(Open5GSEvent &event);
    UserDataIngStatSubscNotificationEvent(const UserDataIngStatSubsc &subscription);
    UserDataIngStatSubscNotificationEvent() = delete;
    UserDataIngStatSubscNotificationEvent(UserDataIngStatSubscNotificationEvent &&other) = delete;
    UserDataIngStatSubscNotificationEvent(const UserDataIngStatSubscNotificationEvent &other) = delete;

    virtual ~UserDataIngStatSubscNotificationEvent() {};

    /* operators */
    UserDataIngStatSubscNotificationEvent &operator=(UserDataIngStatSubscNotificationEvent &&other) = delete;
    UserDataIngStatSubscNotificationEvent &operator=(const UserDataIngStatSubscNotificationEvent &other) = delete;

    const UserDataIngStatSubsc &userDataIngStatSubsc() const;
    void releaseEventData();
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_DISTRIBUTION_SESSION_NOTIFICATION_EVENT_HH_ */
