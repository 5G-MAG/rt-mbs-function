#ifndef _MBSF_DISTRIBUTION_SESSION_INFO_SUBSCRIPTION_HH_
#define _MBSF_DISTRIBUTION_SESSION_INFO_SUBSCRIPTION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Distribution Session Info Subscription class
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

#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include "common.hh"
#include "SubscribedEvents.hh"
#include "Open5GSSBIClient.hh"
#include "openapi/model/Event.h"
#include "openapi/model/SubscribedEvent.h"
#include "openapi/model/EventNotification.h"


namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class Event;
    class SubscribedEvent;
    class EventNotification;
}

MBSF_NAMESPACE_START

class DistributionSessionInfo;
class UserDataIngStatSubsc;
class Open5GSEvent;

class DistributionSessionInfoSubscription {
public:
    using DateTime = std::chrono::system_clock::time_point;

    /* Constructors and Destructor */
    DistributionSessionInfoSubscription(const std::weak_ptr<DistributionSessionInfo> &dist_session, const std::weak_ptr<UserDataIngStatSubsc> &stat_subscription);
    DistributionSessionInfoSubscription() = delete;
    DistributionSessionInfoSubscription(DistributionSessionInfoSubscription &&other);
    DistributionSessionInfoSubscription(const DistributionSessionInfoSubscription &other);

    DistributionSessionInfoSubscription &resetEvents();
    DistributionSessionInfoSubscription &setEvents(std::shared_ptr< reftools::mbsf::Event > event);

    virtual ~DistributionSessionInfoSubscription();

    /* operators */
    DistributionSessionInfoSubscription &operator=(DistributionSessionInfoSubscription &&other);
    DistributionSessionInfoSubscription &operator=(const DistributionSessionInfoSubscription &other);
    bool operator==(const DistributionSessionInfoSubscription &other) const;
    bool operator!=(const DistributionSessionInfoSubscription &other) const { return !(*this == other); };

    /* Getters */
    const int events() const { return m_events; }; /* returns ORed SubscribedEvents::EventTypeBitMask */
    const std::weak_ptr<UserDataIngStatSubsc> &userDataIngStatSubsc() const { return m_userDataIngStatSubsc;};

    void pushNotificationsEvent() const;

private:
    std::weak_ptr<DistributionSessionInfo> m_distributionSession; /* Parent distribution session */
    std::weak_ptr<UserDataIngStatSubsc> m_userDataIngStatSubsc; /* Parent User Data Ing Stat subscription */
    
    int m_events; /* ORed EventTypeBitMask */

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_DISTRIBUTION_SESSION_INFO_SUBSCRIPTION_HH_ */
