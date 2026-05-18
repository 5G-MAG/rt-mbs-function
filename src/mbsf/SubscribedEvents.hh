#ifndef _MBSF_SUBSCRIBED_EVENTS_HH_
#define _MBSF_SUBSCRIBED_EVENTS_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Subscribed Event Timestamps class
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
#include <optional>

#include "common.hh"

namespace reftools::mbsf {
    class DistSessionEventType;
    class DistSessionEventReport;
    class Event;
}

using reftools::mbsf::DistSessionEventType;
using reftools::mbsf::DistSessionEventReport;
using reftools::mbsf::Event;

MBSF_NAMESPACE_START

class SubscribedEvents {
public:
    using DateTime = std::chrono::system_clock::time_point;

typedef enum {
    NONE = 0x0000,
    DATA_INGEST_FAILURE = 0x0001,
    DIST_SESS_TERMINATED = 0x0002,
    DIST_SESS_STARTED = 0x0004,
    DIST_SESS_SERV_MNGT_FAILURE = 0x0008,
    DIST_SESS_STARTING = 0x0010,
    SESSION_TERMINATED = 0x0020,
    USER_DATA_ING_SESS_TERMINATED = 0x0040,
    USER_DATA_ING_SESS_STARTING = 0x0080,
    USER_DATA_ING_SESS_STARTED = 0x0100,
    DIST_SESS_POL_CRTL_FAILURE = 0x0200,
    DELIVERY_STARTED = 0x0400,
    SESSION_STARTED = 0x0800,
    SESSION_RELEASED = 0x1000,
    DIST_SESS_ACTIVATED = 0x2000,
    DIST_SESS_EST_FAILURE = 0x4000,
    USER_SER_AD = 0x8000
} EventTypeBitMask;


 /* event timestamps and EventNotification StatusAddInfo */
#if 0
    std::pair<std::optional<DateTime>, std::optional<std::string>> dataIngestFailure;
    std::optional<DateTime> distSessTerminated;
    std::optional<DateTime> distSessStarted;
    std::optional<DateTime> distSessServMngtFailure;
    std::optional<DateTime> distSessStarting;
    std::optional<DateTime> sessionTerminated;
    std::optional<DateTime> userDataIngSessTerminated;
    std::optional<DateTime> userDataIngSessStarting;
    std::optional<DateTime> userDataIngSessStarted;
    std::optional<DateTime> distSessPolCrtlFailure;
    std::optional<DateTime> deliveryStarted;
    std::optional<DateTime> sessionStarted;
    std::optional<DateTime> sessionReleased;
    std::optional<DateTime> distSessActivated;
    std::optional<DateTime> distSessEstFailure;
    std::optional<DateTime> userSerAd;
#endif
    std::pair<std::optional<DateTime>, std::optional<std::string>> dataIngestFailure {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessTerminated {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessStarted {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessServMngtFailure {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessStarting {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> sessionTerminated {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> userDataIngSessTerminated {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> userDataIngSessStarting {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> userDataIngSessStarted {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessPolCrtlFailure {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> deliveryStarted {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> sessionStarted {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> sessionReleased {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessActivated {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> distSessEstFailure {};
    std::pair<std::optional<DateTime>, std::optional<std::string>> userSerAd {};

    /* Constructors and Destructor */
    SubscribedEvents();
    SubscribedEvents(const SubscribedEvents &other);
    SubscribedEvents(SubscribedEvents &&other);

    virtual ~SubscribedEvents();

    /* operators */
    SubscribedEvents &operator=(SubscribedEvents &&other);
    SubscribedEvents &operator=(const SubscribedEvents &other);
    bool operator==(const SubscribedEvents &other) const;
    bool operator!=(const SubscribedEvents &other) const { return !(*this == other); };

    EventTypeBitMask getEventTypeBitMask(DistSessionEventType dist_session_event_type);
    int updatedSince(const SubscribedEvents &other) const; /* returns event type bits */
    bool isUpdated(std::shared_ptr< Event > status_event, const SubscribedEvents &other) const;
    const std::pair<std::optional<DateTime>, std::optional<std::string>> &timepointForEventType(EventTypeBitMask event_type) const;

    SubscribedEvents &registerEvent(std::shared_ptr<DistSessionEventReport> dist_sess_event_report);
    SubscribedEvents &registerEvent(EventTypeBitMask event_type);
    std::pair<std::optional<DateTime>, std::optional<std::string>> *subscribedEventType(std::shared_ptr< Event > event);
    SubscribedEvents &setSubscribedEventTime(std::shared_ptr< Event > event, std::optional<DateTime> time_point = std::nullopt, std::optional<std::string> status_add_info = std::nullopt);
    const std::pair<std::optional<DateTime>, std::optional<std::string>> &timepointForSubscribedEvent(std::shared_ptr< Event > event) const;

    std::pair<std::optional<DateTime>, std::optional<std::string>> &timepointForEventType(EventTypeBitMask event_type);
    const std::pair<std::optional<DateTime>, std::optional<std::string>> &tpForEventType(EventTypeBitMask event_type) const;

    static bool isSubscribedEventNotificationStimulatedByMbsf(std::shared_ptr< Event > status_event);

    private:
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_SUBSCRIBED_EVENTS_HH_ */
