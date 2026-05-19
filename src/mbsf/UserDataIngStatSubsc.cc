/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: User Data Ingest Session Subscription class
 ******************************************************************************
 * Copyright: (C)2025-2026 British Broadcasting Corporation
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
#include <string>
#include <utility> // for std::swap

#include <uuid/uuid.h>

#include "ogs-sbi.h" // include before "common.hh" to ensure correct logging domain

#include "common.hh"
#include "App.hh"
#include "hash.hh"
#include "ObjManifest.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBIStream.hh"
#include "DistributionSessionNotificationEvent.hh"
#include "UserDataIngSession.hh"
#include "LocalEvents.hh"
#include "UserDataIngSessionNotificationEvent.hh"
#include "openapi/model/MBSUserDataIngStatSubsc.h"
#include "openapi/model/MBSUserDataIngStatSubscPatch.h"
#include "openapi/model/MBSUserDataIngStatNotif.h"
#include "openapi/model/EventNotification.h"
#include "openapi/model/SubscribedEvent.h"
#include "openapi/model/Event.h"
#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"


#include "utilities.hh"

#include "UserDataIngStatSubsc.hh"

using reftools::mbsf::Event;
using reftools::mbsf::EventNotification;
using fiveg_mag_reftools::ProblemCause;
using reftools::mbsf::SubscribedEvent;
using reftools::mbsf::MBSUserDataIngStatSubsc;
using reftools::mbsf::MBSUserDataIngStatSubscPatch;
using reftools::mbsf::MBSUserDataIngStatNotif;
using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbsf_userdataingstatsubsc_api_metadata(
    NMBSF_MBS_UD_INGEST_API_NAME,
    NMBSF_MBS_UD_INGEST_API_VERSION
);

std::recursive_mutex UserDataIngStatSubsc::m_mutex;

static bool check_for_user_data_ing_session_and_distributions_sessions( CJson &subsc);
static int notify_client_callback(int status, ogs_sbi_response_t *response, void *data);
static void send_model_error(const ModelException &err, Open5GSSBIStream &stream, int path_segments, Open5GSSBIMessage &message,
                             const NfServer::AppMetadata &app_meta, const std::optional<NfServer::InterfaceMetadata> &api,
                             const std::string &no_cause_reason, const std::string &log_prefix);


namespace {
    struct RequestData {
        ~RequestData() {};
        const UserDataIngStatSubsc *subscription;
        std::shared_ptr<Open5GSSBIRequest> request;
    };
}

/* Constructors and Destructor */
 UserDataIngStatSubsc::UserDataIngStatSubsc(fiveg_mag_reftools::CJson &json, bool as_request)
    :m_userDataIngSession()
    ,m_generated()
    ,m_lastUsed()
    ,m_hash()
    ,m_subscriptionId()
    ,m_mbsUserDataIngStatSubsc(json, as_request)
    ,m_cache(new UserDataIngStatSubsc::CacheType{})

{
    ogs_uuid_t uuid;

    char id[OGS_UUID_FORMATTED_LENGTH + 1];

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

    std::string json_str(json.serialise());
    m_hash = calculate_hash(std::vector<std::string::value_type>(json_str.begin(), json_str.end()));

    m_subscriptionId = std::string(id);

    setUserDataIngSession();
}

UserDataIngStatSubsc::UserDataIngStatSubsc(const std::weak_ptr<UserDataIngSession> &user_data_ing_session,
                                                                 CJson &json, bool as_request)
    :m_userDataIngSession(user_data_ing_session)
    ,m_subscriptionId()
    ,m_mbsUserDataIngStatSubsc(json, as_request)
    ,m_cache(new UserDataIngStatSubsc::CacheType{})
{
}

UserDataIngStatSubsc::UserDataIngStatSubsc(const std::weak_ptr<UserDataIngSession> &user_data_ing_session,
                                                                 const std::shared_ptr<MBSUserDataIngStatSubsc> &mbs_user_data_ing_stat_subsc)
    :m_userDataIngSession(user_data_ing_session)
    ,m_subscriptionId()
    ,m_mbsUserDataIngStatSubsc(*mbs_user_data_ing_stat_subsc)
    ,m_cache(new UserDataIngStatSubsc::CacheType{})
{
}

UserDataIngStatSubsc::UserDataIngStatSubsc(UserDataIngStatSubsc &&other)
    :m_userDataIngSession(std::move(other.m_userDataIngSession))
    ,m_subscriptionId(std::move(other.m_subscriptionId))
    ,m_mbsUserDataIngStatSubsc(std::move(other.m_mbsUserDataIngStatSubsc))
    ,m_cache(other.m_cache)
{
    other.m_cache = nullptr;
}

UserDataIngStatSubsc::UserDataIngStatSubsc(const UserDataIngStatSubsc &other)
    :m_userDataIngSession(other.m_userDataIngSession)
    ,m_subscriptionId(other.m_subscriptionId)
    ,m_mbsUserDataIngStatSubsc(other.m_mbsUserDataIngStatSubsc)
    ,m_cache(new UserDataIngStatSubsc::CacheType(*other.m_cache))
{
}

UserDataIngStatSubsc::~UserDataIngStatSubsc()
{
    if (m_cache) {
        delete m_cache;
        m_cache = nullptr;
    }
}

/* operators */
UserDataIngStatSubsc &UserDataIngStatSubsc::operator=(UserDataIngStatSubsc &&other)
{
    m_userDataIngSession = std::move(other.m_userDataIngSession);
    m_subscriptionId = std::move(other.m_subscriptionId);
    m_mbsUserDataIngStatSubsc = std::move(other.m_mbsUserDataIngStatSubsc);
    if (m_cache) delete m_cache;
    m_cache = other.m_cache;
    other.m_cache = nullptr;
    return *this;
}

UserDataIngStatSubsc &UserDataIngStatSubsc::operator=(const UserDataIngStatSubsc &other)
{
    m_userDataIngSession = other.m_userDataIngSession;
    m_subscriptionId = other.m_subscriptionId;
    m_mbsUserDataIngStatSubsc = other.m_mbsUserDataIngStatSubsc;
    *m_cache = *other.m_cache;
    return *this;
}

bool UserDataIngStatSubsc::operator==(const UserDataIngStatSubsc &other) const
{
    //if (m_distributionSession != other.m_distributionSession) return false;
    if (m_subscriptionId != other.m_subscriptionId) return false;
    if (m_mbsUserDataIngStatSubsc != other.m_mbsUserDataIngStatSubsc) return false;
    return true;
}

CJson UserDataIngStatSubsc::json(bool as_request = false) const
{
    return m_mbsUserDataIngStatSubsc.toJSON(as_request);
}


const std::string UserDataIngStatSubsc::notifyUri() const
{
    std::string notify_uri = m_mbsUserDataIngStatSubsc.getNotifUri();
    if (notify_uri.empty()) {
        static const std::string empty{};
        return empty;
    }
    return notify_uri;
}

UserDataIngStatSubsc &UserDataIngStatSubsc::modify(CJson &json, bool as_request)
{
    //std::shared_ptr<MBSUserDataIngStatSubscPatch> mbs_user_data_ing_stat_subsc_patch = nullptr;
    //mbs_user_data_ing_stat_subsc_patch.reset(new MBSUserDataIngStatSubscPatch(json, as_request));
    MBSUserDataIngStatSubscPatch mbs_user_data_ing_stat_subsc_patch(json, as_request);
    if (mbs_user_data_ing_stat_subsc_patch.getEventSubscs().has_value()) {
        m_mbsUserDataIngStatSubsc.setEventSubscs(mbs_user_data_ing_stat_subsc_patch.getEventSubscs().value());
    }
    if (mbs_user_data_ing_stat_subsc_patch.getNotifUri().has_value()) {
        m_mbsUserDataIngStatSubsc.setNotifUri(mbs_user_data_ing_stat_subsc_patch.getNotifUri().value());
    }

    resetEventSubscriptions();
    processEventSubscs();
    resetClient();
    return *this;
}

UserDataIngStatSubsc &UserDataIngStatSubsc::update(CJson &json, bool as_request)
{
    MBSUserDataIngStatSubsc tmp(json, as_request);

    if (tmp.getMbsIngSessionId() != m_mbsUserDataIngStatSubsc.getMbsIngSessionId()) {
        throw ModelException("Cannot update MBSUserDataIngStatSubsc.mbsIngSessionId", "UserDataIngStatSubsc", "mbsIngSessionId", ProblemCause::MODIFICATION_NOT_ALLOWED);
    }

    std::swap(m_mbsUserDataIngStatSubsc, tmp);
    resetEventSubscriptions();
    processEventSubscs();
    resetClient();
    return *this;
}


void UserDataIngStatSubsc::setUserDataIngSession()
{
    const std::string &mbs_data_ing_session_id = m_mbsUserDataIngStatSubsc.getMbsIngSessionId();
    static const std::shared_ptr<UserDataIngSession> session = UserDataIngSession::find(mbs_data_ing_session_id);
    m_userDataIngSession = session;
}

std::shared_ptr< EventNotification > UserDataIngStatSubsc::makeEventNotification(std::shared_ptr< Event > event, std::string time_stamp,
                std::optional<std::string > mbs_dist_session_id, std::optional<std::string > status_add_info) const
{
    std::shared_ptr<EventNotification> event_notification = nullptr;

    event_notification.reset(new EventNotification());
    event_notification->setStatusEvent(event);
    event_notification->setMbsDisSessionId(mbs_dist_session_id);
    event_notification->setStatusAddInfo(status_add_info);
    event_notification->setTimeStamp(time_stamp);

    return event_notification;
}

std::shared_ptr<MBSUserDataIngStatNotif > UserDataIngStatSubsc::makeMbsUserDataIngStatNotif(std::list<std::shared_ptr< EventNotification > > event_notifications) const
{
    using NotifType = reftools::mbsf::MBSUserDataIngStatNotif::EventNotifsType;

    NotifType event_notifs;
    event_notifs.clear();

    std::shared_ptr<MBSUserDataIngStatNotif > mbs_user_data_ing_stat_notif = nullptr;
    mbs_user_data_ing_stat_notif.reset(new MBSUserDataIngStatNotif());
    mbs_user_data_ing_stat_notif->setMbsIngSessionId(m_mbsUserDataIngStatSubsc.getMbsIngSessionId());

    for (const auto &event_notification : event_notifications) {
        event_notifs.emplace_back(std::optional<std::shared_ptr<EventNotification>>(event_notification));
    }


    mbs_user_data_ing_stat_notif->setEventNotifs(std::move(event_notifs));

    return mbs_user_data_ing_stat_notif;
}

void UserDataIngStatSubsc::sendNotifications() const
{
    if (!m_cache) return; /* being destroyed */

    ogs_debug("UserDataIngStatSubsc[%p]: Sending notifications", this);
    const auto &notify_uri = m_mbsUserDataIngStatSubsc.getNotifUri();
    if (!notify_uri.empty()) {
        ogs_debug("UserDataIngStatSubsc[%p]: notify URL = %s", this, notify_uri.c_str());
        std::list<std::shared_ptr< EventNotification > > event_notifications = makeEventNotifications();
        if (!event_notifications.empty()) {
            {
                ogs_debug("UserDataIngStatSubsc[%p]: building notify request", this);
                std::shared_ptr<MBSUserDataIngStatNotif > stat_notif = makeMbsUserDataIngStatNotif(event_notifications);
                CJson json = stat_notif->toJSON(true);
                std::string body(json.serialise());
                ogs_debug("UserDataIngStatSubsc[%p]: SENDING: %s", this, body.c_str());

            }
            ogs_debug("UserDataIngStatSubsc[%p]: have events to send", this);
            if (!m_cache->client) {
                ogs_debug("UserDataIngStatSubsc[%p]: create new client", this);
                m_cache->notifyUri = notify_uri;
                m_cache->client.reset(new Open5GSSBIClient(notify_uri));
            }
            ogs_debug("UserDataIngStatSubsc[%p]: building notify request", this);
            std::shared_ptr<MBSUserDataIngStatNotif > stat_notif = makeMbsUserDataIngStatNotif(event_notifications);
            CJson json = stat_notif->toJSON(true);
            std::string body(json.serialise());
            ogs_debug("UserDataIngStatSubsc[%p]: sending: %s", this, body.c_str());
            static const std::string post_method(OGS_SBI_HTTP_METHOD_POST);
            static const std::string api_version(std::format("{}/{}", MBSUserDataIngStatNotif::apiName, MBSUserDataIngStatNotif::apiVersion));
            std::shared_ptr<Open5GSSBIRequest> request = nullptr;

            request.reset(new Open5GSSBIRequest(post_method, notify_uri, api_version,
                                                body, OGS_SBI_CONTENT_JSON_TYPE));
           // std::shared_ptr<Open5GSSBIRequest> request(new Open5GSSBIRequest(post_method, notify_uri, api_version,
            //                                    body, OGS_SBI_CONTENT_JSON_TYPE));
            RequestData *data = new RequestData{this, request};
            request->setOwner(false);
            m_cache->client->sendRequest(notify_client_callback, request, data);
        }
    }
}

void UserDataIngStatSubsc::resetClient()
{
    if (!m_cache) return;
    const auto &notify_uri = m_mbsUserDataIngStatSubsc.getNotifUri();
    if (!notify_uri.empty() && !m_cache->notifyUri.empty() && m_cache->client) {
        m_cache->notifyUri = notify_uri;
        m_cache->client.reset(new Open5GSSBIClient(notify_uri));
    }
}

void UserDataIngStatSubsc::checkAndSetUserDataIngSessStartedTerminatedEvent(std::shared_ptr<UserDataIngSession> user_data_ing_session) const
{
    if (!user_data_ing_session) return;
    const std::string &user_data_ing_session_id = user_data_ing_session->userDataIngSessionId();
    const std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &user_data_ing_stat_subscs = App::self().context()->userDataIngStatSubscs();
    for(const auto &user_data_ing_stat_subsc : user_data_ing_stat_subscs) {
        const std::shared_ptr<UserDataIngStatSubsc> &stat_subsc = user_data_ing_stat_subsc.second;
        const std::string &id = stat_subsc->userDataIngSessionId();
        if (user_data_ing_session_id == id && user_data_ing_session->checkIfAllMBSDistributionSessionsEstablished()) {

            std::shared_ptr< Event > event = nullptr;
            event.reset(new Event());
            *event = Event::VAL_USER_DATA_ING_SESS_STARTED;

            std::optional<SubscribedEvents::DateTime> tp = user_data_ing_session->timeOfLatestDistributionSessionEvent(SubscribedEvents::DIST_SESS_STARTING);

            if (tp.has_value()) {
                stat_subsc->setSubscribedEventTime(event, tp.value());
                user_data_ing_session->resetMBSDistributionSessionsEstablishedFlag();
            }
            //stat_subsc->setSubscribedEventTime(event, DateTime::clock::now());
        }
        if (user_data_ing_session_id == id && user_data_ing_session->checkIfAllMBSDistributionSessionsTerminated()) {

            std::shared_ptr< Event > ev = nullptr;
            ev.reset(new Event());


            std::shared_ptr< Event > e = nullptr;
            e.reset(new Event());

            *e = Event::VAL_SESSION_TERMINATED;

            *ev = Event::VAL_USER_DATA_ING_SESS_TERMINATED;

            std::optional<SubscribedEvents::DateTime> tp = user_data_ing_session->timeOfLatestDistributionSessionEvent(SubscribedEvents::DIST_SESS_TERMINATED);
            if (tp.has_value()) {
                stat_subsc->setSubscribedEventTime(ev, tp.value());
                stat_subsc->setSubscribedEventTime(e, tp.value());
                user_data_ing_session->resetMBSDistributionSessionsTerminatedFlag();
            }
        }

    }
}

std::list<std::shared_ptr< EventNotification > > UserDataIngStatSubsc::makeEventNotifications() const
{
    std::list<std::shared_ptr< EventNotification > > result;
    std::shared_ptr<UserDataIngSession> user_data_ing_session(m_userDataIngSession.lock());

    if (m_cache && user_data_ing_session) {
        //std::list<std::optional<std::shared_ptr< SubscribedEvent > >
        const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = m_mbsUserDataIngStatSubsc.getEventSubscs();
        for (const auto &subscribed_event : subscribed_events) {
            if (!subscribed_event) continue;
            std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
            if (!subsc_event) continue;
            /*
            if (SubscribedEvents::isSubscribedEventNotificationStimulatedByMbsf(subsc_event->getStatusEvent()))
            {
                const std::optional<SubscribedEvents::DateTime> &timepoint = m_cache->lastReportedEventTimes.timepointForSubscribedEvent(subsc_event->getStatusEvent());
                if (!timepoint.has_value()) continue;
                const std::string &timestamp = time_point_to_iso8601_utc_str(timepoint.value());
                std::shared_ptr< EventNotification > notification = makeEventNotification(subsc_event->getStatusEvent(), timestamp, std::nullopt, std::nullopt);
                result.push_back(std::move(notification));
            }
            */
            checkAndSetUserDataIngSessStartedTerminatedEvent(user_data_ing_session);

            if (SubscribedEvents::isSubscribedEventNotificationStimulatedByMbsf(subsc_event->getStatusEvent()) &&
                            m_subscribedEventTimestamps.isUpdated(subsc_event->getStatusEvent(), m_cache->lastReportedEventTimes))

            {

                const std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>> &subs_event = m_subscribedEventTimestamps.timepointForSubscribedEvent(subsc_event->getStatusEvent());
                if (!subs_event.first.has_value()) continue;
                auto evt = m_cache->lastReportedEventTimes.subscribedEventType(subsc_event->getStatusEvent());
                if (evt) {
                    *evt = subs_event;
                }
                const std::string &timestamp = time_point_to_iso8601_utc_str(subs_event.first.value());
                std::shared_ptr< EventNotification > notification = makeEventNotification(subsc_event->getStatusEvent(), timestamp, std::nullopt, subs_event.second);
                result.push_back(std::move(notification));
            }

            const std::optional<std::string > &mbs_dist_session_id = subsc_event->getMbsDistSessionId();
            if (!mbs_dist_session_id.has_value()) continue;
            const std::string &dist_session_id = mbs_dist_session_id.value();
            const std::shared_ptr< Event > status_event= subsc_event->getStatusEvent();
            if (!status_event) continue;

            std::shared_ptr< UserDataIngSession::ContextData > context_data = user_data_ing_session->getDistributionSessionInfoData(dist_session_id);
            /* get list of registered events from the DistributionSession in the User Data Ing Session */
            const auto &dist_sess_event_timestamps = context_data->distributionSessionInfo->eventTimestamps();

            if (dist_sess_event_timestamps.isUpdated(status_event, m_cache->lastReportedEventTimes)) {
                const std::pair<std::optional<SubscribedEvents::DateTime>, std::optional<std::string>> &time_point = dist_sess_event_timestamps.timepointForSubscribedEvent(subsc_event->getStatusEvent());
                if (!time_point.first.has_value()) continue;
                // get pointer to the appropriate variable std::optional<SubscribedEvents::DateTime> *;
                auto p = m_cache->lastReportedEventTimes.subscribedEventType(subsc_event->getStatusEvent());
                if (p) {
                    *p = time_point;
                }
                const std::string &time_stamp = time_point_to_iso8601_utc_str(time_point.first.value());
                std::shared_ptr< EventNotification > event_notification = makeEventNotification(status_event, time_stamp, dist_session_id, std::nullopt);
                result.push_back(std::move(event_notification));
            }
        }

    }
    return result;

}

static bool check_for_user_data_ing_session_and_distributions_sessions(CJson &subsc)
{
    std::shared_ptr<MBSUserDataIngStatSubsc> mbs_user_data_ing_stat_subsc = nullptr;
    std::shared_ptr<UserDataIngSession> ing_session = nullptr;
    mbs_user_data_ing_stat_subsc.reset(new MBSUserDataIngStatSubsc(subsc, true));
    const std::string &mbs_user_data_ing_session_id = mbs_user_data_ing_stat_subsc->getMbsIngSessionId();
    try {
        ing_session = UserDataIngSession::find(mbs_user_data_ing_session_id);
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << mbs_user_data_ing_session_id << "] does not exist.";
        ogs_error("%s", err.str().c_str());
        return false;
    }

    const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = mbs_user_data_ing_stat_subsc->getEventSubscs();
    for (const auto &subscribed_event : subscribed_events) {
        if (!subscribed_event) continue;
        std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
        if (!subsc_event) continue;
        const std::optional<std::string > mbs_dist_session_id = subsc_event->getMbsDistSessionId();
        if (!mbs_dist_session_id.has_value()) continue;
        std::shared_ptr< UserDataIngSession::ContextData > context_data = ing_session->getDistributionSessionInfoData(mbs_dist_session_id.value());
        if (!context_data) return false;
    }
    return true;
}

static int notify_client_callback(int status, ogs_sbi_response_t *response, void *data)
{
    //ogs_event_t *e = ogs_event_new(OGS_EVENT_SBI_CLIENT);
    ogs_event_t *e = nullptr;
    int rv;
    RequestData *req_data = reinterpret_cast<RequestData*>(data);

    e = ogs_event_new(OGS_EVENT_SBI_CLIENT);
    ogs_assert(e);
    e->sbi.request = req_data->request->ogsSBIRequest();
    e->sbi.response = response;
    e->sbi.data = data;
    e->sbi.state = status;

    if (status != OGS_OK) {
        ogs_log_message(status == OGS_DONE ? OGS_LOG_DEBUG : OGS_LOG_WARN, 0,
                        "MBSUserDataIngStatNotif failed [%d]", status);
        if(req_data) {
            if(req_data->request) {
	        req_data->request->setOwner(true);
                req_data->request.reset();
	    }
            delete req_data;
	}
	if(response) ogs_sbi_response_free(response);
        if(e) ogs_event_free(e);
	return OGS_ERROR;
    } else {
        ogs_assert(response);
    }

    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed:%d", (int)rv);
        ogs_sbi_response_free(response);
        ogs_event_free(e);
        return OGS_ERROR;
    }

    return OGS_OK;
}

bool UserDataIngStatSubsc::processEvent(Open5GSEvent &event)
{

    const NfServer::InterfaceMetadata &nmbsf_mbs_userdataingstatsubsc_api = g_nmbsf_userdataingstatsubsc_api_metadata;
    const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();

    switch (event.id()) {
    case OGS_EVENT_SBI_SERVER:
        {

            Open5GSSBIRequest request(event.sbiRequest());

            Open5GSSBIMessage message;
            ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData()));
            Open5GSSBIStream stream(stream_id);

            Open5GSSBIServer server(stream.server());
            std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("Failed to parse request headers");
                return false;
            }

            std::shared_ptr<Open5GSSBIRequest> request_ctx(new Open5GSSBIRequest(message));

            std::string service_name(message.serviceName());
            std::string resource0(message.resourceComponent(0));
            ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", service_name.c_str(), resource0.c_str());
            if (service_name == "nmbsf-mbs-ud-ingest") {
                api.emplace(nmbsf_mbs_userdataingstatsubsc_api);
            } else {
                message.resetHeader();
                return false;
            }

            if (api.value() == nmbsf_mbs_userdataingstatsubsc_api) {
                /******** nmbsf-mbs-ud-ingest ********/
                std::string api_version(message.apiVersion());
                if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Unsupported API version [%s]", api_version.c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta,
                                                           api, "Unsupported API version"));
                    return true;
                }

                if (resource0 == "status-subscriptions") {
                    std::string method(message.method());
                    const char *ptr_resource1 = message.resourceComponent(1);
                    if (ptr_resource1) {
                        /* .../status-subscriptions/{subscriptionId}... */
                        std::string subscription_id(ptr_resource1);
                        std::shared_ptr<UserDataIngStatSubsc> user_data_ing_stat_subsc = nullptr;

                        try {
                            //user_data_ing_stat_subsc = UserDataIngStatSubsc::find(subscription_id);
                            user_data_ing_stat_subsc = UserDataIngStatSubsc::locate(subscription_id);
                        } catch (std::out_of_range &ex) {
                            std::ostringstream err;
                            err << "User Data Ing Status Subscription [" << subscription_id << "] not found: "
                                << ex.what();
                            ogs_error("%s", err.str().c_str());
                            std::map<std::string, std::string> invalid_params(NfServer::makeInvalidParams("{subscriptionId}", ex.what()));
                            ogs_assert(true == NfServer::sendError(stream, ProblemCause::SUBSCRIPTION_NOT_FOUND, 4, message, app_meta, api,
                                "User Data Ing Status Subscription not found", err.str(), std::nullopt, invalid_params));
                            return true;
                        }
                        if (method == OGS_SBI_HTTP_METHOD_GET) {
                             user_data_ing_stat_subsc->sendResponse(stream, api, app_meta);
                            return true;
                        } else if (method == OGS_SBI_HTTP_METHOD_PUT) {
                                   user_data_ing_stat_subsc->subscriptionUpdate(stream, message, request, api, app_meta);
                            return true;

                        } else if (method == OGS_SBI_HTTP_METHOD_PATCH) {
                            user_data_ing_stat_subsc->subscriptionPatch(stream, message, request, api, app_meta);
                            return true;
                        } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
                             user_data_ing_stat_subsc->subscriptionDelete(stream, message, request, api, app_meta);
                            return true;
                        } else if (method == OGS_SBI_HTTP_METHOD_OPTIONS) {
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, OGS_SBI_HTTP_METHOD_GET ", " OGS_SBI_HTTP_METHOD_PUT ", " OGS_SBI_HTTP_METHOD_PATCH ", " OGS_SBI_HTTP_METHOD_DELETE ", " OGS_SBI_HTTP_METHOD_OPTIONS, api, app_meta));
                            NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                            return true;

                        }
                        else {
                            std::ostringstream err;

                            err << "Invalid method [" << message.method() << "] for " << message.serviceName() << "/"
                                << message.apiVersion() << "/" << message.resourceComponent(0);
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                app_meta, api, "Bad request", err.str()));
                            return true;
                        }

                    } else {
                        /* is .../status-subscriptions */
                        /* POST method allowed by TS 29.580 */
                        /* Also include OPTIONS method for RESTful introspection */

                        if (method == OGS_SBI_HTTP_METHOD_POST) {
                            ogs_debug("POST response: status = %i", message.resStatus());
                            std::shared_ptr<UserDataIngStatSubsc> user_data_ing_stat_subsc = nullptr;
                            ogs_debug("Request body: %s", request.content());
                            if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                                return true;
                            }
                            CJson subsc(CJson::Null);
                            try {
                                subsc = CJson::parse(request.content());
                            } catch (std::exception &ex) {
                                static const char *err = "Unable to parse MBSF User Data Ing Session Stat Subsc as JSON.";
                                ogs_error("%s", err);
                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Data Ingest Stat Subsc", err));
                                return true;
                            }

                            {
                                std::string txt(subsc.serialise());
                                ogs_debug("Request Parsed JSON: %s", txt.c_str());
                            }

                            if (!check_for_user_data_ing_session_and_distributions_sessions(subsc)) {
                                std::ostringstream err;
                                err << "Invalid User Data Ing Session or Distribution session present in User Data Ingest Stat Subsc";
                                ogs_error("%s", err.str().c_str());
                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 3, message, app_meta,
                                                            api, "Bad request", err.str()));
                                return true;

                            }

                            try {

                                user_data_ing_stat_subsc.reset(new UserDataIngStatSubsc(subsc, true));
                            } catch (ModelException &ex) {
                                if (ex.cause) {
                                    ogs_assert(true == NfServer::sendError(stream, ex.cause.value(), 3, message, app_meta,
                                                    api, /*ex.cause.value().reason() */ "Mandatory information element missing", ex.what(), std::nullopt, std::nullopt));
                                 } else {
                                     ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 3, message,
                                                   app_meta, api, "Mandatory information element missing", ex.what(), std::nullopt, std::nullopt));
                                 }
                                 return true;
                            }

                            App::self().context()->addUserDataIngStatSubsc(user_data_ing_stat_subsc);
                            user_data_ing_stat_subsc->processEventSubscs();

                            CJson user_data_ing_stat_subsc_json(user_data_ing_stat_subsc->json(false));
                            std::string body(user_data_ing_stat_subsc_json.serialise());
                            ogs_debug("Response Parsed JSON: %s", body.c_str());
                            std::ostringstream location;
                            location << request.uri() << "/" << user_data_ing_stat_subsc->subscriptionId();
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                                body.empty()?nullptr:"application/json",
                                user_data_ing_stat_subsc->generated(),
                                user_data_ing_stat_subsc->hash().c_str(),
                                App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                std::nullopt/*nullptr*/, api,  App::self().mbsfAppMetadata()));
                                ogs_assert(response);
                                NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_CREATED);
                                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));

                            return true;
                        } else if (method == OGS_SBI_HTTP_METHOD_OPTIONS) {
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, OGS_SBI_HTTP_METHOD_POST ", " OGS_SBI_HTTP_METHOD_OPTIONS, api, app_meta));
                            NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                            return true;

                        } else {
                            std::ostringstream err;

                            err << "Invalid method [" << message.method() << "] for " << message.serviceName() << "/"
                                << message.apiVersion() << "/" << message.resourceComponent(0);
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                app_meta, api, "Bad request", err.str()));
                            return true;
                        }
                    }

                }  else {
                    return false;
                    /*
                    std::ostringstream err;
                    err << "Unknown object type \"" << message.resourceComponent(0) << "\" in MBSF User Data Ingest Stat Subsc";
                    ogs_error("%s", err.str().c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message, app_meta,
                                                            api, "Bad request", err.str()));
                    return true;
                    */
                }

            } else {
                static const char *err = "Missing service name from URL path";
                ogs_error("%s", err);
                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta, std::nullopt,
                                                "Missing service name", err));
            }
            return true;

        }

    case OGS_EVENT_SBI_CLIENT:
        {
            for(auto &[user_data_ing_stat_subsc_id, user_data_ing_stat_subsc] : App::self().context()->userDataIngStatSubscs()) {
                if (user_data_ing_stat_subsc->processClientResponse(event)) return true;
            }
            return false;
            //break;
        }


     case LocalEvents::SEND_NOTIFICATION:
        {
            DistributionSessionNotificationEvent dist_event(event);
            const auto &subsc = dist_event.distributionSessionInfoSubscription();
            ogs_debug("Sending notifications for subscription %p", &subsc);
            const std::weak_ptr<UserDataIngStatSubsc> stat_subsc = subsc.userDataIngStatSubsc();
            if (auto sp = stat_subsc.lock()) {
                // sp is std::shared_ptr<UserDataIngStatSubsc>
                sp->sendNotifications(); // call the method on the shared_ptr
            } else {
                ogs_debug("subscription object expired; handle accordingly");
            }
            dist_event.releaseEventData();
            return true;
        }
        case LocalEvents::SEND_USER_DATA_ING_SESS_NOTIFICATION:
        {
            UserDataIngSessionNotificationEvent user_data_ing_session_event(event);
            const auto &user_data_ing_session = user_data_ing_session_event.userDataIngSession();
            ogs_debug("Sending notifications for ing session %p", &user_data_ing_session);
            const std::string &user_data_ing_session_id = user_data_ing_session.userDataIngSessionId();
            const std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &user_data_ing_stat_subscs = App::self().context()->userDataIngStatSubscs();

	    for(const auto &user_data_ing_stat_subsc : user_data_ing_stat_subscs) {
                const std::shared_ptr<UserDataIngStatSubsc> &stat_subsc = user_data_ing_stat_subsc.second;
                const std::string &id = stat_subsc->userDataIngSessionId();
                if (user_data_ing_session_id == id && stat_subsc->checkForUserDataIngSessStarting()) {
                    std::shared_ptr< Event > event = nullptr;
                    event.reset(new Event());
                    *event = Event::VAL_USER_DATA_ING_SESS_STARTING;
                    stat_subsc->setSubscribedEventTime(event, DateTime::clock::now());
                    stat_subsc->sendNotifications();

                }

		if (user_data_ing_session_id == id && stat_subsc->checkForUserServiceAnn()) {
		    std::list<std::string> object_locators = user_data_ing_session.getObjectLocators();
                    if(!object_locators.size() || user_data_ing_session.userSerAdNotificationSent()) break;
		    for(const std::string &object_locator : object_locators) {
                        std::shared_ptr< Event > event = nullptr;
                        event.reset(new Event());
                        *event = Event::VAL_USER_SER_AD;
                        stat_subsc->setSubscribedEventTime(event, DateTime::clock::now(), object_locator);
                        stat_subsc->sendNotifications();
		    }
		    user_data_ing_session.userSerAdNotificationSent(true);

                }

            }

            user_data_ing_session_event.releaseEventData();
            return true;
        }

        default:
            return false;

    }
    return false;
}

void UserDataIngStatSubsc::setSubscribedEventTime(std::shared_ptr< Event > event, std::optional<DateTime> time_point, std::optional<std::string> status_add_info)
{
     m_subscribedEventTimestamps.setSubscribedEventTime(event, time_point, status_add_info);
}

bool UserDataIngStatSubsc::processClientResponse(const Open5GSEvent &event)
{
    switch (event.id()) {
    case OGS_EVENT_SBI_CLIENT:
    {
        const void *raw = event.sbiData();
        if (!raw) {
            break;
        }

        // Best-effort: check whether raw looks like a valid pointer to a RequestData
        uintptr_t p = reinterpret_cast<uintptr_t>(raw);
        bool looks_like_pointer = (p > 0x1000) && (p % alignof(RequestData) == 0);

        // If it looks like a valid pointer, use it directly (preserve original behavior).
        if (looks_like_pointer) {
            RequestData *req_data = reinterpret_cast<RequestData*>(const_cast<void*>(raw));
            if (req_data && req_data->subscription == this) {
                if (event.sbiState() == OGS_OK) {
                    auto resp = event.sbiResponse(true);
                    ogs_debug("Got %i response from notification(s) to %s", resp.status(), req_data->request->uri());
                } else {
                    ogs_debug("Problem sending notification(s) to %s", req_data->request->uri());
                }

                req_data->request->setOwner(true);
                req_data->request.reset();
                delete req_data;

                return true;
            }
            // If pointer doesn't match this subscription, fall through and return false.
            break;
        }


        break;
    }
    default:
        break;
    }
    return false;
}



bool UserDataIngStatSubsc::checkForUserDataIngSessStarting() {
    //std::list<std::optional<std::shared_ptr< SubscribedEvent > >
    const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = m_mbsUserDataIngStatSubsc.getEventSubscs();
    for (const auto &subscribed_event : subscribed_events) {
        if (!subscribed_event) continue;
        std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
        if (!subsc_event) continue;
        std::shared_ptr< Event > event = subsc_event->getStatusEvent();
        if (event->getValue() == Event::VAL_USER_DATA_ING_SESS_STARTING) return true;
    }
    return false;
}

bool UserDataIngStatSubsc::checkForUserServiceAnn() {
    //std::list<std::optional<std::shared_ptr< SubscribedEvent > >
    const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = m_mbsUserDataIngStatSubsc.getEventSubscs();
    for (const auto &subscribed_event : subscribed_events) {
        if (!subscribed_event) continue;
        std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
        if (!subsc_event) continue;
        std::shared_ptr< Event > event = subsc_event->getStatusEvent();
        if (event->getValue() == Event::VAL_USER_SER_AD) {
	    return true;
	}
    }

    return false;
}


const std::string &UserDataIngStatSubsc::userDataIngSessionId() const {
    return m_mbsUserDataIngStatSubsc.getMbsIngSessionId();
}


void UserDataIngStatSubsc::processEventSubscs() {
    std::shared_ptr<UserDataIngSession> user_data_ing_session(m_userDataIngSession.lock());
    //std::list<std::optional<std::shared_ptr< SubscribedEvent > >
    const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = m_mbsUserDataIngStatSubsc.getEventSubscs();
    for (const auto &subscribed_event : subscribed_events) {
        if (!subscribed_event) continue;
        std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
        if (!subsc_event) continue;
        const std::optional<std::string > mbs_dist_session_id = subsc_event->getMbsDistSessionId();
        if (!mbs_dist_session_id.has_value()) continue;
        std::string dist_session_id = mbs_dist_session_id.value();
        std::shared_ptr< Event > event = subsc_event->getStatusEvent();
        std::shared_ptr< UserDataIngSession::ContextData > context_data = user_data_ing_session->getDistributionSessionInfoData(dist_session_id);
        if (!context_data) continue;
        std::shared_ptr<DistributionSessionInfo> distribution_session_info = context_data->distributionSessionInfo;
        distribution_session_info->addEventSubscription(weak_from_this(), event);
    }

}

void UserDataIngStatSubsc::resetEventSubscriptions() {
    std::shared_ptr<UserDataIngSession> user_data_ing_session(m_userDataIngSession.lock());
    //std::list<std::optional<std::shared_ptr< SubscribedEvent > >
    const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = m_mbsUserDataIngStatSubsc.getEventSubscs();
    for (const auto &subscribed_event : subscribed_events) {
        if (!subscribed_event) continue;
        std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
        if (!subsc_event) continue;
        const std::optional<std::string > mbs_dist_session_id = subsc_event->getMbsDistSessionId();
        if (!mbs_dist_session_id.has_value()) continue;
        std::string dist_session_id = mbs_dist_session_id.value();
        std::shared_ptr< UserDataIngSession::ContextData > context_data = user_data_ing_session->getDistributionSessionInfoData(dist_session_id);
        if (!context_data) continue;
        std::shared_ptr<DistributionSessionInfo> distribution_session_info = context_data->distributionSessionInfo;
        distribution_session_info->resetEventSubscription();
    }

}

void UserDataIngStatSubsc::removeEventSubscriptions() {
    std::shared_ptr<UserDataIngSession> user_data_ing_session(m_userDataIngSession.lock());
    //std::list<std::optional<std::shared_ptr< SubscribedEvent > >
    const reftools::mbsf::MBSUserDataIngStatSubsc::EventSubscsType &subscribed_events = m_mbsUserDataIngStatSubsc.getEventSubscs();
    for (const auto &subscribed_event : subscribed_events) {
        if (!subscribed_event) continue;
        std::shared_ptr<SubscribedEvent> subsc_event = *subscribed_event;
        if (!subsc_event) continue;
        const std::optional<std::string > mbs_dist_session_id = subsc_event->getMbsDistSessionId();
        if (!mbs_dist_session_id.has_value()) continue;
        std::string dist_session_id = mbs_dist_session_id.value();
        std::shared_ptr< UserDataIngSession::ContextData > context_data = user_data_ing_session->getDistributionSessionInfoData(dist_session_id);
        if (!context_data) continue;
        std::shared_ptr<DistributionSessionInfo> distribution_session_info = context_data->distributionSessionInfo;
        distribution_session_info->removeEventSubscription();
    }

}



const std::shared_ptr<UserDataIngStatSubsc> &UserDataIngStatSubsc::find(const std::string &id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &UserDataIngStatSubscs = App::self().context()->userDataIngStatSubscs();
    auto it = UserDataIngStatSubscs.find(id);
    if (it == UserDataIngStatSubscs.end()) {
        throw std::out_of_range("MBS User Stat Subsc not found");
    }
    return it->second;
}

const std::shared_ptr<UserDataIngStatSubsc> &UserDataIngStatSubsc::locate(const std::string &id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &UserDataIngStatSubscs = App::self().context()->userDataIngStatSubscs();
    auto it = UserDataIngStatSubscs.find(id);
    if (it == UserDataIngStatSubscs.end()) {
        throw std::out_of_range("MBS User Stat Subsc not found");
    }

    if (!it->second->hasUserDataIngSession()) {
        it->second->removeEventSubscriptions();
        App::self().context()->deleteUserDataIngStatSubsc(id);
        throw std::out_of_range("MBS Subscription not found");
    }
    return it->second;
}

bool UserDataIngStatSubsc::hasUserDataIngSession() {
    std::shared_ptr<UserDataIngSession> ing_session = nullptr;
    const std::string &mbs_user_data_ing_session_id = m_mbsUserDataIngStatSubsc.getMbsIngSessionId();
    try {
        ing_session = UserDataIngSession::find(mbs_user_data_ing_session_id);
        if (ing_session) return true;
    } catch (const std::out_of_range &e) {
        std::ostringstream err;
        err << "MBS User Data Ingest Session [" << mbs_user_data_ing_session_id << "] does not exist.";
        ogs_error("%s", err.str().c_str());
        return false;
    }

    return true;
}

void UserDataIngStatSubsc::subscriptionPatch(Open5GSSBIStream &stream, Open5GSSBIMessage &message, Open5GSSBIRequest &request,
                               const std::optional<NfServer::InterfaceMetadata> &api,
                               const NfServer::AppMetadata &app_meta)
{

    if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/merge-patch+json") {
        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                1, message, app_meta, api, "Unsupported Media Type", "Expected content type: application/merge-patch+json"));
        return;
    }

    CJson req_json(CJson::Null);
    try {
        req_json = CJson::parse(request.content());
        //updateSubscription(req_json, true);
        modify(req_json, true);
        sendResponse(stream, api, app_meta);
    } catch (ModelException &ex) {
        send_model_error(ex, stream, 1, message, app_meta, api, "Bad Request",
                            "Unable to parse MBS User Data Ing Status Subscription Patch as JSON");
        return;
    }
}

void UserDataIngStatSubsc::subscriptionUpdate(Open5GSSBIStream &stream, Open5GSSBIMessage &message, Open5GSSBIRequest &request,
                               const std::optional<NfServer::InterfaceMetadata> &api,
                               const NfServer::AppMetadata &app_meta)
{

    CJson req_json(CJson::Null);
    try {
        req_json = CJson::parse(request.content());
        update(req_json, true);
        sendResponse(stream, api, app_meta);
    } catch (ModelException &ex) {
        send_model_error(ex, stream, 1, message, app_meta, api, "Bad Request",
                            "Unable to parse MBS User Data Ing Status Subscription Patch as JSON");
        return;
    }
}

void UserDataIngStatSubsc::subscriptionDelete(Open5GSSBIStream &stream, Open5GSSBIMessage &message, Open5GSSBIRequest &request,
                                const std::optional<NfServer::InterfaceMetadata> &api, const NfServer::AppMetadata &app_meta)
{
    removeEventSubscriptions();
    App::self().context()->deleteUserDataIngStatSubsc(m_subscriptionId);
    std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0,
                                                                        std::nullopt, api, app_meta));
    ogs_assert(response);
    NfServer::populateResponse(response, std::string(), OGS_SBI_HTTP_STATUS_NO_CONTENT);
    ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
}



void UserDataIngStatSubsc::sendResponse(Open5GSSBIStream &stream, const std::optional<NfServer::InterfaceMetadata> &api,
                               const NfServer::AppMetadata &app_meta)
{
    std::string body(m_mbsUserDataIngStatSubsc.toJSON(false).serialise());
    std::optional<std::string> content_type;
    if (!body.empty()) content_type = "application/json";
    std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, content_type,
                            std::nullopt /* last-modified */, std::nullopt /* etag */,
                            App::self().context()->cacheControl.defaultMaxAge,
                            std::nullopt, api, app_meta));
    ogs_assert(response);
    NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_OK);
    ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
}

static void send_model_error(const ModelException &err, Open5GSSBIStream &stream, int path_segments, Open5GSSBIMessage &message,
                             const NfServer::AppMetadata &app_meta, const std::optional<NfServer::InterfaceMetadata> &api,
                             const std::string &no_cause_reason, const std::string &log_prefix)
{
    std::ostringstream error_oss;
    std::ostringstream oss;
    std::optional<std::map<std::string,std::string> > invalid_params = std::nullopt;

    if (!err.parameter.empty()) {
        invalid_params = std::map<std::string,std::string>{ {err.parameter, err.what()} };
        error_oss << err.parameter << ": ";
    }
    error_oss << err.what();
    const std::string &error = error_oss.str();

    if (err.cause) {
        auto cause = err.cause.value();
        oss << cause.reason() << ": " << error;
        ogs_assert(true == NfServer::sendError(stream, cause, path_segments, message, app_meta, api, cause.reason(), error,
                                                std::nullopt, invalid_params));
    } else {
        oss << no_cause_reason << ": " << error;
        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, path_segments, message, app_meta, api,
                                                no_cause_reason, error));
    }
    ogs_error("%s: %s", log_prefix.c_str(), oss.str().c_str());
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
