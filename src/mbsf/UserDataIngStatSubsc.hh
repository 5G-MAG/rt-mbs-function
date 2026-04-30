#ifndef _MBSF_USER_DATA_ING_SESSION_SUBSCRIPTION_HH_
#define _MBSF_USER_DATA_ING_SESSION_SUBSCRIPTION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: User Data Ingest Session Subscription class
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
#include <mutex>

#include "common.hh"
#include "SubscribedEvents.hh"
#include "Open5GSSBIClient.hh"
#include "openapi/model/MBSUserDataIngStatSubsc.h"

namespace reftools::mbsf {
    class EventNotification;
    class Event;
    class MBSUserDataIngStatSubsc;
    class MBSUserDataIngStatNotif;
}


MBSF_NAMESPACE_START

class UserDataIngSession;
class Open5GSEvent;

class UserDataIngStatSubsc: public std::enable_shared_from_this<UserDataIngStatSubsc> {

public:

    using SysTimeMS = std::chrono::system_clock::time_point;
    using DateTime = std::chrono::system_clock::time_point;

    /* Constructors and Destructor */

    UserDataIngStatSubsc(fiveg_mag_reftools::CJson &json, bool as_request);

    UserDataIngStatSubsc(const std::weak_ptr<UserDataIngSession> &user_data_ing_session, fiveg_mag_reftools::CJson &json,
                                    bool as_request);

    UserDataIngStatSubsc(const std::weak_ptr<UserDataIngSession> &user_data_ing_session,
                                                                 const std::shared_ptr<reftools::mbsf::MBSUserDataIngStatSubsc> &mbs_user_data_ing_stat_subsc);

    UserDataIngStatSubsc() = delete;
    UserDataIngStatSubsc(UserDataIngStatSubsc &&other);
    UserDataIngStatSubsc(const UserDataIngStatSubsc &other);

    virtual ~UserDataIngStatSubsc();

    /* operators */
    UserDataIngStatSubsc &operator=(UserDataIngStatSubsc &&other);
    UserDataIngStatSubsc &operator=(const UserDataIngStatSubsc &other);
    bool operator==(const UserDataIngStatSubsc &other) const;
    bool operator!=(const UserDataIngStatSubsc &other) const { return !(*this == other); };

    fiveg_mag_reftools::CJson json(bool as_request) const;
    static const std::shared_ptr<UserDataIngStatSubsc> &find(const std::string &id);
    static const std::shared_ptr<UserDataIngStatSubsc> &locate(const std::string &id);

    bool hasUserDataIngSession();

    /* Getters */
    const std::string &subscriptionId() const { return m_subscriptionId; };
    const reftools::mbsf::MBSUserDataIngStatSubsc &getMBSUserIngStatSubsc() const {return m_mbsUserDataIngStatSubsc;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};

    const std::string &userDataIngSessionId() const;

    const int eventTypes() const { return m_eventTypes; }; /* returns ORed SubscribedEvents::EventTypeBitMask */
    const std::string notifyUri() const;
    const std::optional<std::string> &nfcInstanceId() const;

    /* Setters */
    void setSubscribedEventTime(std::shared_ptr< reftools::mbsf::Event > event, std::optional<SubscribedEvents::DateTime> time_point = std::nullopt, std::optional<std::string> status_add_info = std::nullopt);
    UserDataIngStatSubsc &modify(fiveg_mag_reftools::CJson &json, bool as_request=false);
    UserDataIngStatSubsc &update(fiveg_mag_reftools::CJson &json, bool as_request=false);

    /* OpenAPI type constructors */
    const reftools::mbsf::MBSUserDataIngStatSubsc &mbsUserDataIngStatSubsc() const { return m_mbsUserDataIngStatSubsc; };
    std::list<std::shared_ptr< reftools::mbsf::EventNotification > > makeEventNotifications() const;
    std::shared_ptr< reftools::mbsf::EventNotification > makeEventNotification(std::shared_ptr< reftools::mbsf::Event > event, std::string time_stamp, std::optional<std::string > mbs_dist_session_id, std::optional<std::string > status_add_info = std::nullopt) const;

    std::shared_ptr<reftools::mbsf::MBSUserDataIngStatNotif > makeMbsUserDataIngStatNotif(std::list<std::shared_ptr< reftools::mbsf::EventNotification > > event_notifications) const;
    bool checkForUserDataIngSessStarting();
    bool checkForUserServiceAnn();
    void checkAndSetUserDataIngSessStartedTerminatedEvent(std::shared_ptr<UserDataIngSession> user_data_ing_session) const;

    /** Send Notifications to client
     * This will send Event Notifications to the notifyUri location.
     */
    void sendNotifications() const;

    bool processClientResponse(const Open5GSEvent &event);

    void subscriptionPatch(Open5GSSBIStream &stream, Open5GSSBIMessage &message, Open5GSSBIRequest &request,
                               const std::optional<NfServer::InterfaceMetadata> &api,
                               const NfServer::AppMetadata &app_meta);

    void subscriptionUpdate(Open5GSSBIStream &stream, Open5GSSBIMessage &message, Open5GSSBIRequest &request,
                               const std::optional<NfServer::InterfaceMetadata> &api,
                               const NfServer::AppMetadata &app_meta);
    void subscriptionDelete(Open5GSSBIStream &stream, Open5GSSBIMessage &message, Open5GSSBIRequest &request,
                                const std::optional<NfServer::InterfaceMetadata> &api,
                                const NfServer::AppMetadata &app_meta);
    void processEventSubscs();

    static bool processEvent(Open5GSEvent &event);
    static std::recursive_mutex m_mutex;

private:
    void resetEventSubscriptions();
    void removeEventSubscriptions();
    void setUserDataIngSession();
    void resetClient();
    void sendResponse(Open5GSSBIStream &stream, const std::optional<NfServer::InterfaceMetadata> &api,
                               const NfServer::AppMetadata &app_meta);


    std::weak_ptr<UserDataIngSession> m_userDataIngSession; /* Associated User Data Ingest Session */
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_subscriptionId;
    int m_eventTypes; /* ORed EventTypeBitMask */
    reftools::mbsf::MBSUserDataIngStatSubsc m_mbsUserDataIngStatSubsc;
    SubscribedEvents m_subscribedEventTimestamps;

    struct CacheType {
        CacheType() : lastReportedEventTimes(), client(), notifyUri() {};
        CacheType(const CacheType &other) : lastReportedEventTimes(other.lastReportedEventTimes), client() {};
        CacheType(CacheType &&other) : lastReportedEventTimes(std::move(other.lastReportedEventTimes)), client(std::move(other.client)) {};
        CacheType &operator=(const CacheType &other) {lastReportedEventTimes = other.lastReportedEventTimes; client.reset(); return *this; };
        CacheType &operator=(CacheType &&other) {lastReportedEventTimes = std::move(other.lastReportedEventTimes); client = std::move(other.client); return *this; };
        SubscribedEvents lastReportedEventTimes;
        std::unique_ptr<Open5GSSBIClient> client;
        std::string notifyUri;
    } *m_cache;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_DISTRIBUTION_SESSION_SUBSCRIPTION_HH_ */
