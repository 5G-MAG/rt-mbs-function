/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Subscribed Events
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
#include <exception>
#include <memory>
#include <optional>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <ctime>
#include <utility>

#include "openapi/model/DistSessionEventType.h"
#include "openapi/model/DistSessionEventReport.h"
#include "openapi/model/Event.h"

#include "ogs-sbi.h" // include before "common.hh" to ensure correct logging domain

#include "common.hh"
#include "App.hh"

#include "utilities.hh"

#include "SubscribedEvents.hh"

MBSF_NAMESPACE_START


/* Constructors and Destructor */

SubscribedEvents::SubscribedEvents()
    :dataIngestFailure()
    ,distSessTerminated()
    ,distSessStarted()
    ,distSessServMngtFailure()
    ,distSessStarting()
    ,sessionTerminated()
    ,userDataIngSessTerminated()
    ,userDataIngSessStarting()
    ,userDataIngSessStarted()
    ,distSessPolCrtlFailure()
    ,deliveryStarted()
    ,sessionStarted()
    ,sessionReleased()
    ,distSessActivated()
    ,distSessEstFailure()
    ,userSerAd()
{
}

SubscribedEvents::SubscribedEvents(const SubscribedEvents &other)
    :dataIngestFailure(other.dataIngestFailure)
    ,distSessTerminated(other.distSessTerminated)
    ,distSessStarted(other.distSessStarted)
    ,distSessServMngtFailure(other.distSessServMngtFailure)
    ,distSessStarting(other.distSessStarting)
    ,sessionTerminated(other.sessionTerminated)
    ,userDataIngSessTerminated(other.userDataIngSessTerminated)
    ,userDataIngSessStarting(other.userDataIngSessStarting)
    ,userDataIngSessStarted(other.userDataIngSessStarted)
    ,distSessPolCrtlFailure(other.distSessPolCrtlFailure)
    ,deliveryStarted(other.deliveryStarted)
    ,sessionStarted(other.sessionStarted)
    ,sessionReleased(other.sessionReleased)
    ,distSessActivated(other.distSessActivated)
    ,distSessEstFailure(other.distSessEstFailure)
    ,userSerAd(other.userSerAd)
{
}

SubscribedEvents::SubscribedEvents(SubscribedEvents &&other)
    :dataIngestFailure(std::move(other.dataIngestFailure))
    ,distSessTerminated(std::move(other.distSessTerminated))
    ,distSessStarted(std::move(other.distSessStarted))
    ,distSessServMngtFailure(std::move(other.distSessServMngtFailure))
    ,distSessStarting(std::move(other.distSessStarting))
    ,sessionTerminated(std::move(other.sessionTerminated))
    ,userDataIngSessTerminated(std::move(other.userDataIngSessTerminated))
    ,userDataIngSessStarting(std::move(other.userDataIngSessStarting))
    ,userDataIngSessStarted(std::move(other.userDataIngSessStarted))
    ,distSessPolCrtlFailure(std::move(other.distSessPolCrtlFailure))
    ,deliveryStarted(std::move(other.deliveryStarted))
    ,sessionStarted(std::move(other.sessionStarted))
    ,sessionReleased(std::move(other.sessionReleased))
    ,distSessActivated(std::move(other.distSessActivated))
    ,distSessEstFailure(std::move(other.distSessEstFailure))
    ,userSerAd(std::move(other.userSerAd))
{
}



SubscribedEvents::~SubscribedEvents()
{
}

/* operators */
SubscribedEvents &SubscribedEvents::operator=(SubscribedEvents &&other)
{
    userDataIngSessStarting = std::move(other.userDataIngSessStarting);
    userDataIngSessStarted = std::move(other.userDataIngSessStarted);
    userDataIngSessTerminated = std::move(other.userDataIngSessTerminated);
    distSessStarting = std::move(other.distSessStarting);
    distSessStarted = std::move(other.distSessStarted);
    distSessTerminated = std::move(other.distSessTerminated);
    distSessServMngtFailure = std::move(other.distSessServMngtFailure);
    distSessPolCrtlFailure = std::move(other.distSessPolCrtlFailure);
    dataIngestFailure = std::move(other.dataIngestFailure);
    deliveryStarted = std::move(other.deliveryStarted);
    sessionTerminated = std::move(other.sessionTerminated);
    sessionStarted = std::move(other.sessionStarted);
    sessionReleased = std::move(other.sessionReleased);
    distSessActivated = std::move(other.distSessActivated);
    distSessEstFailure = std::move(other.distSessEstFailure);
    userSerAd = std::move(other.userSerAd);
    return *this;
}

SubscribedEvents &SubscribedEvents::operator=(const SubscribedEvents &other)
{

    userDataIngSessStarting = other.userDataIngSessStarting;
    userDataIngSessStarted = other.userDataIngSessStarted;
    userDataIngSessTerminated = other.userDataIngSessTerminated;
    distSessStarting = other.distSessStarting;
    distSessStarted = other.distSessStarted;
    distSessTerminated = other.distSessTerminated;
    distSessServMngtFailure = other.distSessServMngtFailure;
    distSessPolCrtlFailure = other.distSessPolCrtlFailure;
    dataIngestFailure = other.dataIngestFailure;
    deliveryStarted = other.deliveryStarted;
    sessionTerminated = other.sessionTerminated;
    sessionStarted = other.sessionStarted;
    sessionReleased = other.sessionReleased;
    distSessActivated = other.distSessActivated;
    distSessEstFailure = other.distSessEstFailure;
    userSerAd = other.userSerAd;
    return *this;
}

bool SubscribedEvents::operator==(const SubscribedEvents &other) const
{
    return userDataIngSessStarting == other.userDataIngSessStarting &&
           userDataIngSessStarted == other.userDataIngSessStarted &&
           userDataIngSessTerminated == other.userDataIngSessTerminated &&
           distSessStarting == other.distSessStarting &&
           distSessStarted == other.distSessStarted &&
           distSessTerminated == other.distSessTerminated &&
           distSessServMngtFailure == other.distSessServMngtFailure &&
           distSessPolCrtlFailure == other.distSessPolCrtlFailure &&
           dataIngestFailure == other.dataIngestFailure &&
           deliveryStarted == other.deliveryStarted &&
           sessionTerminated == other.sessionTerminated &&
           sessionStarted == other.sessionStarted &&
           sessionReleased == other.sessionReleased &&
           distSessActivated == other.distSessActivated &&
           distSessEstFailure == other.distSessEstFailure &&
           userSerAd == other.userSerAd;
}

bool SubscribedEvents::isUpdated(std::shared_ptr< Event > status_event, const SubscribedEvents &other) const {
 const std::pair<std::optional<DateTime>, std::optional<std::string>> &time_point = timepointForSubscribedEvent(status_event);
 const std::pair<std::optional<DateTime>, std::optional<std::string>> &other_time_point = other.timepointForSubscribedEvent(status_event);
 if (time_point.first.has_value() &&(!other_time_point.first.has_value() || other_time_point.first.value() < time_point.first.value())) {
     return true;
 }
 return false;

}

bool SubscribedEvents::isSubscribedEventNotificationStimulatedByMbsf(std::shared_ptr< Event > status_event) {
    switch (status_event->getValue()) {
    case Event::VAL_USER_DATA_ING_SESS_TERMINATED:
        return true;
    case Event::VAL_USER_DATA_ING_SESS_STARTING:
        return true;
    case Event::VAL_USER_DATA_ING_SESS_STARTED:
        return true;
    case Event::VAL_USER_SER_AD:
        return true;
    default:
        return false;
    }

    return false;

}


int SubscribedEvents::updatedSince(const SubscribedEvents &other) const
{
    int event_types = 0;

     if (dataIngestFailure.first && (!other.dataIngestFailure.first || other.dataIngestFailure.first.value() < dataIngestFailure.first.value()))
        event_types |= DATA_INGEST_FAILURE;

    if (distSessTerminated.first && (!other.distSessTerminated.first || other.distSessTerminated.first.value() < distSessTerminated.first.value()))
        event_types |= DIST_SESS_TERMINATED;

    if (distSessStarted.first && (!other.distSessStarted.first || other.distSessStarted.first.value() < distSessStarted.first.value()))
        event_types |= DIST_SESS_STARTED;

    if (distSessServMngtFailure.first && (!other.distSessServMngtFailure.first || other.distSessServMngtFailure.first.value() < distSessServMngtFailure.first.value()))
        event_types |= DIST_SESS_SERV_MNGT_FAILURE;

    if (distSessStarting.first && (!other.distSessStarting.first || other.distSessStarting.first.value() < distSessStarting.first.value()))
        event_types |= DIST_SESS_STARTING;

    if (sessionTerminated.first && (!other.sessionTerminated.first || other.sessionTerminated.first.value() < sessionTerminated.first.value()))
        event_types |= SESSION_TERMINATED;

    if (userDataIngSessTerminated.first && (!other.userDataIngSessTerminated.first || other.userDataIngSessTerminated.first.value() < userDataIngSessTerminated.first.value()))
        event_types |= USER_DATA_ING_SESS_TERMINATED;

    if (userDataIngSessStarting.first && (!other.userDataIngSessStarting.first || other.userDataIngSessStarting.first.value() < userDataIngSessStarting.first.value()))
        event_types |= USER_DATA_ING_SESS_STARTING;

    if (userDataIngSessStarted.first && (!other.userDataIngSessStarted.first || other.userDataIngSessStarted.first.value() < userDataIngSessStarted.first.value()))
        event_types |= USER_DATA_ING_SESS_STARTED;

    if (distSessPolCrtlFailure.first && (!other.distSessPolCrtlFailure.first || other.distSessPolCrtlFailure.first.value() < distSessPolCrtlFailure.first.value()))
        event_types |= DIST_SESS_POL_CRTL_FAILURE;

    if (deliveryStarted.first && (!other.deliveryStarted.first || other.deliveryStarted.first.value() < deliveryStarted.first.value()))
        event_types |= DELIVERY_STARTED;

    if (sessionStarted.first && (!other.sessionStarted.first || other.sessionStarted.first.value() < sessionStarted.first.value()))
        event_types |= SESSION_STARTED;

    if (sessionReleased.first && (!other.sessionReleased.first || other.sessionReleased.first.value() < sessionReleased.first.value()))
        event_types |= SESSION_RELEASED;

    if (distSessActivated.first && (!other.distSessActivated.first || other.distSessActivated.first.value() < distSessActivated.first.value()))
        event_types |= DIST_SESS_ACTIVATED;

    if (distSessEstFailure.first && (!other.distSessEstFailure.first || other.distSessEstFailure.first.value() < distSessEstFailure.first.value()))
        event_types |= DIST_SESS_EST_FAILURE;

    if (userSerAd.first && (!other.userSerAd.first || other.userSerAd.first.value() < userSerAd.first.value()))
        event_types |= USER_SER_AD;

    return event_types;
}

const std::pair<std::optional<SubscribedEvents::DateTime>, std::optional< std::string>> &SubscribedEvents::timepointForEventType(EventTypeBitMask event_type) const
{
    return tpForEventType(event_type);
}

SubscribedEvents &SubscribedEvents::registerEvent(EventTypeBitMask event_type)
{
    auto &tp = timepointForEventType(event_type);
    tp.first.emplace(DateTime::clock::now());
    tp.second.reset();
   // return tp;
   return *this;
}

SubscribedEvents &SubscribedEvents::setSubscribedEventTime(std::shared_ptr< Event > event, std::optional<DateTime> time_point, std::optional<std::string> status_add_info)
{
    auto p = subscribedEventType(event);
    if (p) {
        if (time_point) {
            p->first.emplace(*time_point);
        } else {
            p->first.reset();
        }
	if(status_add_info) {
	    p->second.emplace(*status_add_info);
	} else {
            p->second.reset();
	}

    } else {
        ogs_info("Invalid Subscribed Event Type");
    }
    return *this;
}

SubscribedEvents &SubscribedEvents::registerEvent(std::shared_ptr<DistSessionEventReport> dist_sess_event_report)
{
    std::shared_ptr< DistSessionEventType > distribution_session_event_type = dist_sess_event_report->getEventType();
    const std::optional<std::string > &time_stamp = dist_sess_event_report->getTimeStamp();
    SubscribedEvents::EventTypeBitMask event_type = getEventTypeBitMask(*distribution_session_event_type);
    auto &tp = timepointForEventType(event_type);
    if (time_stamp.has_value()) {
        tp.first.emplace(to_time_point_iso8601(time_stamp.value()));
        if (tp.first == std::chrono::system_clock::time_point{}) {
            ogs_info("epoch parse failed");
            tp.first.emplace(DateTime::clock::now());
        }
	tp.second.reset();
    } else {
        tp.first.emplace(DateTime::clock::now());
	tp.second.reset();
    }
    return *this;
}

std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>> *SubscribedEvents::subscribedEventType(std::shared_ptr< Event > event)
{
    switch (event->getValue()) {
    case Event::VAL_DATA_INGEST_FAILURE:
        return &this->dataIngestFailure;
    case Event::VAL_DIST_SESS_TERMINATED:
        return &this->distSessTerminated;
    case Event::VAL_DIST_SESS_STARTED:
        return &this->distSessStarted;
    case Event::VAL_DIST_SESS_SERV_MNGT_FAILURE:
        return &this->distSessServMngtFailure;
    case Event::VAL_DIST_SESS_STARTING:
        return &this->distSessStarting;
    case Event::VAL_SESSION_TERMINATED:
        return &this->sessionTerminated;
    case Event::VAL_USER_DATA_ING_SESS_TERMINATED:
        return &this->userDataIngSessTerminated;
    case Event::VAL_USER_DATA_ING_SESS_STARTING:
        return &this->userDataIngSessStarting;
    case Event::VAL_USER_DATA_ING_SESS_STARTED:
        return &this->userDataIngSessStarted;
    case Event::VAL_DIST_SESS_POL_CRTL_FAILURE:
        return &this->distSessPolCrtlFailure;
    case Event::VAL_DELIVERY_STARTED:
        return &this->deliveryStarted;
    case Event::VAL_SESSION_STARTED:
        return &this->sessionStarted;
    case Event::VAL_SESSION_RELEASED:
        return &this->sessionReleased;
    case Event::VAL_DIST_SESS_ACTIVATED:
        return &this->distSessActivated;
    case Event::VAL_DIST_SESS_EST_FAILURE:
        return &this->distSessEstFailure;
    case Event::VAL_USER_SER_AD:
        return &this->userSerAd;
    default:
        return nullptr;
    }
    return nullptr;
}

const std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>> &SubscribedEvents::timepointForSubscribedEvent(std::shared_ptr< Event > event) const
{
    switch (event->getValue()) {
    case Event::VAL_DATA_INGEST_FAILURE:
        return dataIngestFailure;
    case Event::VAL_DIST_SESS_TERMINATED:
        return distSessTerminated;
    case Event::VAL_DIST_SESS_STARTED:
        return distSessStarted;
    case Event::VAL_DIST_SESS_SERV_MNGT_FAILURE:
        return distSessServMngtFailure;
    case Event::VAL_DIST_SESS_STARTING:
        return distSessStarting;
    case Event::VAL_SESSION_TERMINATED:
        return sessionTerminated;
    case Event::VAL_USER_DATA_ING_SESS_TERMINATED:
        return userDataIngSessTerminated;
    case Event::VAL_USER_DATA_ING_SESS_STARTING:
        return userDataIngSessStarting;
    case Event::VAL_USER_DATA_ING_SESS_STARTED:
        return userDataIngSessStarted;
    case Event::VAL_DIST_SESS_POL_CRTL_FAILURE:
        return distSessPolCrtlFailure;
    case Event::VAL_DELIVERY_STARTED:
        return deliveryStarted;
    case Event::VAL_SESSION_STARTED:
        return sessionStarted;
    case Event::VAL_SESSION_RELEASED:
        return sessionReleased;
    case Event::VAL_DIST_SESS_ACTIVATED:
        return distSessActivated;
    case Event::VAL_DIST_SESS_EST_FAILURE:
        return distSessEstFailure;
    case Event::VAL_USER_SER_AD:
        return userSerAd;
    default:
        ogs_warn("Ignoring unknown Event: %s", event->getString().c_str());
        break;
    }
    throw std::range_error("Bad SubscribedEvent given to SubscribedEvents::timepointForSubscribedEvent()");

}

/*** private: ***/
std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>> &SubscribedEvents::timepointForEventType(EventTypeBitMask event_type)
{
    return const_cast<std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>>&>(tpForEventType(event_type));
}

const std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>> &SubscribedEvents::tpForEventType(EventTypeBitMask event_type) const
{
    switch (event_type) {
    case DATA_INGEST_FAILURE:
        return dataIngestFailure;
    case DIST_SESS_TERMINATED:
        return distSessTerminated;
    case DIST_SESS_STARTED:
        return distSessStarted;
    case  DIST_SESS_SERV_MNGT_FAILURE:
        return distSessServMngtFailure;
    case DIST_SESS_STARTING:
        return distSessStarting;
    case SESSION_TERMINATED:
        return sessionTerminated;
    case USER_DATA_ING_SESS_TERMINATED:
        return userDataIngSessTerminated;
    case USER_DATA_ING_SESS_STARTING:
        return userDataIngSessStarting;
    case USER_DATA_ING_SESS_STARTED:
        return userDataIngSessStarted;
    case DIST_SESS_POL_CRTL_FAILURE:
        return distSessPolCrtlFailure;
    case DELIVERY_STARTED:
        return deliveryStarted;
    case SESSION_STARTED:
        return sessionStarted;
    case SESSION_RELEASED:
        return sessionReleased;
    case DIST_SESS_ACTIVATED:
        return distSessActivated;
    case DIST_SESS_EST_FAILURE:
        return distSessEstFailure;
    case USER_SER_AD:
        return userSerAd;
    default:
        break;
    }
    throw std::range_error("Bad event type given to SubscribedEvents::timepointForEventType()");
}

SubscribedEvents::EventTypeBitMask SubscribedEvents::getEventTypeBitMask(DistSessionEventType dist_session_event_type)
{
    switch (dist_session_event_type) {
    case DistSessionEventType::NO_VAL:
        return NONE;
    case DistSessionEventType::VAL_DATA_INGEST_FAILURE:
        return DATA_INGEST_FAILURE;
    case DistSessionEventType::VAL_SESSION_DEACTIVATED:
        return DIST_SESS_TERMINATED;
    //case DistSessionEventType::VAL_SESSION_ESTABLISHED:
    //    return DIST_SESS_STARTED;
    case DistSessionEventType::VAL_SESSION_ACTIVATED:
        return DIST_SESS_ACTIVATED;
    case DistSessionEventType::VAL_SERVICE_MANAGEMENT_FAILURE:
        return DIST_SESS_SERV_MNGT_FAILURE;
    case DistSessionEventType::VAL_DATA_INGEST_SESSION_ESTABLISHED:
        return DIST_SESS_STARTING;
    case DistSessionEventType::VAL_DATA_INGEST_SESSION_TERMINATED:
        return SESSION_TERMINATED;
    default:
        return NONE;
    }
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
