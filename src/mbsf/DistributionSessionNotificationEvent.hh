#ifndef _MBSF_DISTRIBUTION_SESSION_NOTIFICATION_EVENT_HH_
#define _MBSF_DISTRIBUTION_SESSION_NOTIFICATION_EVENT_HH_
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
#include "Open5GSEvent.hh"

MBSF_NAMESPACE_START

class DistributionSessionInfoSubscription;

class DistributionSessionNotificationEvent : public Open5GSEvent {
public:
    /* Constructors and Destructor */
    DistributionSessionNotificationEvent(Open5GSEvent &event);
    DistributionSessionNotificationEvent(const DistributionSessionInfoSubscription &dist_session);
    DistributionSessionNotificationEvent() = delete;
    DistributionSessionNotificationEvent(DistributionSessionNotificationEvent &&other) = delete;
    DistributionSessionNotificationEvent(const DistributionSessionNotificationEvent &other) = delete;

    virtual ~DistributionSessionNotificationEvent() {};

    /* operators */
    DistributionSessionNotificationEvent &operator=(DistributionSessionNotificationEvent &&other) = delete;
    DistributionSessionNotificationEvent &operator=(const DistributionSessionNotificationEvent &other) = delete;

    const DistributionSessionInfoSubscription &distributionSessionInfoSubscription() const;
    void releaseEventData();
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_DISTRIBUTION_SESSION_NOTIFICATION_EVENT_HH_ */
