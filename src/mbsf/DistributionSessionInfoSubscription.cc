/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Distribution Session Info Subscription class
 ******************************************************************************
 * Copyright: (C)2024-2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin2@bbc.co.uk>
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
#include <string>

#include <uuid/uuid.h>

#include "ogs-sbi.h" // include before "common.hh" to ensure correct logging domain

#include "common.hh"
#include "App.hh"
#include "DistributionSessionInfo.hh"
#include "UserDataIngStatSubsc.hh"
#include "DistributionSessionNotificationEvent.hh"
#include "openapi/model/Event.h"
#include "openapi/model/SubscribedEvent.h"
#include "openapi/model/EventNotification.h"
#include "utilities.hh"

#include "DistributionSessionInfoSubscription.hh"

using reftools::mbsf::Event;
using reftools::mbsf::EventNotification;
using reftools::mbsf::SubscribedEvent;
using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

/* Constructors and Destructor */
DistributionSessionInfoSubscription::DistributionSessionInfoSubscription(const std::weak_ptr<DistributionSessionInfo> &dist_session, const std::weak_ptr<UserDataIngStatSubsc> &stat_subscription)
    :m_distributionSession(dist_session)
    ,m_userDataIngStatSubsc(stat_subscription)
    ,m_events(0)
{
}

DistributionSessionInfoSubscription::DistributionSessionInfoSubscription(DistributionSessionInfoSubscription &&other)
    :m_distributionSession(std::move(other.m_distributionSession))
    ,m_events(std::move(other.m_events))
{
}

DistributionSessionInfoSubscription::DistributionSessionInfoSubscription(const DistributionSessionInfoSubscription &other)
    :m_distributionSession(other.m_distributionSession)
    ,m_events(other.m_events)
{
}

DistributionSessionInfoSubscription::~DistributionSessionInfoSubscription()
{
}

/* operators */
DistributionSessionInfoSubscription &DistributionSessionInfoSubscription::operator=(DistributionSessionInfoSubscription &&other)
{
    m_distributionSession = std::move(other.m_distributionSession);
    return *this;
}

DistributionSessionInfoSubscription &DistributionSessionInfoSubscription::operator=(const DistributionSessionInfoSubscription &other)
{
    m_distributionSession = other.m_distributionSession;
    m_events = other.m_events;
    return *this;
}

bool DistributionSessionInfoSubscription::operator==(const DistributionSessionInfoSubscription &other) const
{
    //if (m_distributionSession != other.m_distributionSession) return false;
    if (m_events != other.m_events) return false;
    return true;
}

void DistributionSessionInfoSubscription::pushNotificationsEvent() const
{
    std::shared_ptr<Open5GSEvent> event(new DistributionSessionNotificationEvent(*this));
    App::self().ogsApp()->pushEvent(event);
}

DistributionSessionInfoSubscription &DistributionSessionInfoSubscription::resetEvents()
{
    m_events = 0;
    return *this;

}


DistributionSessionInfoSubscription &DistributionSessionInfoSubscription::setEvents(std::shared_ptr< reftools::mbsf::Event > event)
{
    switch (event->getValue()) {
    case Event::VAL_DATA_INGEST_FAILURE:
        m_events |= SubscribedEvents::DATA_INGEST_FAILURE;
        break;
    case Event::VAL_DIST_SESS_TERMINATED:
        m_events |= SubscribedEvents::DIST_SESS_TERMINATED;
        break;
    case Event::VAL_DIST_SESS_STARTED:
        m_events |= SubscribedEvents::DIST_SESS_STARTED;
        break;
    case Event::VAL_DIST_SESS_SERV_MNGT_FAILURE:
        m_events |= SubscribedEvents::DIST_SESS_SERV_MNGT_FAILURE;
        break;
    case Event::VAL_DIST_SESS_STARTING:
        m_events |= SubscribedEvents::DIST_SESS_STARTING;
        break;
    case Event::VAL_SESSION_TERMINATED:
        m_events |= SubscribedEvents::SESSION_TERMINATED;
        break;
    case Event::VAL_USER_DATA_ING_SESS_TERMINATED:
        m_events |= SubscribedEvents::USER_DATA_ING_SESS_TERMINATED;
        break;
    case Event::VAL_USER_DATA_ING_SESS_STARTING:
        m_events |= SubscribedEvents::USER_DATA_ING_SESS_STARTING;
        break;
    case Event::VAL_USER_DATA_ING_SESS_STARTED:
        m_events |= SubscribedEvents::USER_DATA_ING_SESS_STARTED;
        break;
    case Event::VAL_DIST_SESS_POL_CRTL_FAILURE:
        m_events |= SubscribedEvents::DIST_SESS_POL_CRTL_FAILURE;
        break;
    case Event::VAL_DELIVERY_STARTED:
        m_events |= SubscribedEvents::DELIVERY_STARTED;
        break;
    case Event::VAL_SESSION_STARTED:
        m_events |= SubscribedEvents::SESSION_STARTED;
        break;
    case Event::VAL_SESSION_RELEASED:
        m_events |= SubscribedEvents::SESSION_RELEASED;
        break;
    case Event::VAL_DIST_SESS_ACTIVATED:
        m_events |= SubscribedEvents::DIST_SESS_ACTIVATED;
        break;
    case Event::VAL_DIST_SESS_EST_FAILURE:
        m_events |= SubscribedEvents::DIST_SESS_EST_FAILURE;
        break;
    case Event::VAL_USER_SER_AD:
        m_events |= SubscribedEvents::USER_SER_AD;
        break;
    default:
        ogs_warn("Ignoring unknown Event: %s", event->getString().c_str());
        break;
    }
    return *this;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
