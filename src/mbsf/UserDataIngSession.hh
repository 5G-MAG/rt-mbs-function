# ifndef _MBSF_USER_DATA_ING_SESSION_HH_
#define _MBSF_USER_DATA_ING_SESSION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS User Data Ingest Session class
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "mb-smf-service-consumer.h"

#include <any>
#include <chrono>
#include <functional>
#include <memory>
#include <tuple>
#include <mutex>
#include <set>

#include <rtsdp/SDP.hh>

#include "openapi/model/DistSessionState.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "common.hh"
#include "AlwaysActive.hh"
#include "ActivePeriods.hh"
#include "ActivePeriodsRepRule.hh"
#include "DistributionSessionInfo.hh"
#include "SubscribedEvents.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class DistSession;
    class PacketDistrMethInfo;
    class Ssm;
}

MBSF_NAMESPACE_START

class ActivePeriods;
class AlwaysActive;
class CarouselObject;
class MBSMFMBSSession;
class ObjManifest;
class Open5GSEvent;
class Open5GSSBINFInstance;
class Open5GSSBIRequest;
class Open5GSSBIObject;
class Open5GSTimer;
class ServiceScheduleDesc;
class UserService;
class UserServiceAnnBundle;
class UserServiceDesc;

class UserDataIngSession : public std::enable_shared_from_this<UserDataIngSession> {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;
    using DateTime = std::chrono::system_clock::time_point;
    using ActPeriodsType = reftools::mbsf::MBSUserDataIngSession::ActPeriodsType;
    using ActPeriodsRepRuleType = reftools::mbsf::MBSUserDataIngSession::ActPeriodsRepRuleType;

    //Pair containing User Data Ing Session Id and key for the Dist Session Info
    using UserDataIngDistSessId = std::pair<std::string, std::string>;

    //Pair containing Distribution Session ID sent to the MBSTF and the above UserDataIngDistSessId
    using SessionIdContainer = std::pair<std::string, std::shared_ptr< UserDataIngDistSessId >>;

    enum class MBSSessionState {
        NO = 0,       // no session
        CREATED,      // session successfully created
        DELETED,      // session successfully deleted
        FAILED        // session creation failed
    };

    struct ContextData {
        std::string ingSessionId;
        std::string distSessionInfoKey;
        std::shared_ptr<DistributionSessionInfo> distributionSessionInfo;
        std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> info;
        std::shared_ptr<reftools::mbsf::Ssm> ssm;
        in_port_t ssm_port;
        std::shared_ptr<Open5GSSBIRequest> request;
        ogs_pool_id_t streamId;
        std::shared_ptr<MBSMFMBSSession> MBSSession = nullptr;
        MBSSessionState MBSSessionStatus = MBSSessionState::NO;
        std::optional<fiveg_mag_reftools::ProblemCause> mbsmfProblemCause = std::nullopt;
        std::optional<fiveg_mag_reftools::CJson> mbsmfProblemDetailJson = std::nullopt;
        bool receivedMBSTFResponse = false;
        bool receivedMBSTFPatchResponse = false;
        bool patchUpdateSucceded = false;
        bool stateUpdate = false;
        bool needsUpdate = false;
        bool markForDeletion = false;
        std::string distSessionId = std::string{};
        bool MBSTFDistSessionDeleted = false;
        std::string mbstfNFInstanceId = std::string{};
        std::string mbstfDistSessionId = std::string{};
        bool distSessionState = false;
        mb_smf_sc_tmgi_t *tmgi = nullptr;
        std::string mbstfNotificationUrl = std::string{};
        uint64_t tsi;
        reftools::mbsf::DistSessionState last_requested_state;
        reftools::mbsf::DistSessionState last_reported_state;
        std::shared_ptr<reftools::mbsf::DistSession> distSession = nullptr;
        std::shared_ptr<LIBRTSDP_NAMESPACE_NAME(SDP)> sdp = nullptr;
    };

    UserDataIngSession(fiveg_mag_reftools::CJson &json, bool as_request);
    UserDataIngSession(const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &mbs_user_service);
    UserDataIngSession() = delete;
    UserDataIngSession(UserDataIngSession &&other) = delete;
    UserDataIngSession(const UserDataIngSession &other) = delete;
    UserDataIngSession &operator=(UserDataIngSession &&other) = delete;
    UserDataIngSession &operator=(const UserDataIngSession &other) = delete;

    UserDataIngSession(const std::string &user_data_ing_session_id, const std::string &mbs_user_service_id,
                    const std::map<std::string, std::shared_ptr< DistributionSessionInfo > > &distribution_session_infos);

    virtual ~UserDataIngSession();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    enum OgsExtendedEventId : int {
        MBSF_LOCAL_SEND_MBSTF_REQ_BUILD = OGS_MAX_NUM_OF_PROTO_EVENT + 1600,
        MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION,
        MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK,
        MBSF_LOCAL_SEND_MBSTF_PATCH_BUILD,
        MBSF_LOCAL_SEND_MBSTF_CAROUSE_OBJECT_MANIFEST_BUILD
    };

    const std::string &userDataIngSessionId() const { return m_UserDataIngSessionId; };
    const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &getMBSUserIngSession() const {return m_MBSUserDataIngSession;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};
    const int32_t serviceScheduleDescVersion() {return m_serviceScheduleDescriptionVersion++;};
    //const std::shared_ptr<ObjManifest> carouselObjectManifest() const {return m_carouselObjectManifest;};
    const std::unique_ptr<Open5GSSBIObject> &getSbiObject() const {return m_sbiObject;};
    const std::shared_ptr<UserServiceAnnBundle> getUserServiceAnnBundler() const { return m_userServiceAnnBundle;};
    const bool isUserServiceAnnBundleAvailable() const { return m_userServiceAnnBundleAvailable;};
    const bool isIncludedInCarouselObjectManifest() const { return m_includedInCarouselObjectManifest;};
    const bool userSerAdNotificationSent() const {return m_userSerAdNotificationSent;};
    ogs_sbi_xact_t *nmbstfDiscoverOnly(const std::shared_ptr<ContextData> &data);
    ogs_sbi_xact_t *nmbstfDiscoverAndSend(const std::shared_ptr<UserDataIngDistSessId> &ids, ogs_sbi_build_f build, void *context, void *data);
    UserDataIngSession &setNFInstance(ogs_sbi_service_type_e service_type, ogs_sbi_nf_instance_t *nf_instance);

    UserDataIngSession &alwaysActive() {m_activePeriods.reset(new AlwaysActive(m_UserDataIngSessionId)); return *this;};
    UserDataIngSession &activePeriods(const ActPeriodsType &act_periods) {m_activePeriods.reset(new ActivePeriods(act_periods, m_activePeriods, *this)); return *this;};
    UserDataIngSession &activePeriodsRepRule(const ActPeriodsRepRuleType &act_periods_rep_rule) {m_activePeriods.reset(new ActivePeriodsRepRule(act_periods_rep_rule, m_activePeriods, *this)); return *this;};
    UserDataIngSession &userServiceAnnBundleAvailable(bool available) { m_userServiceAnnBundleAvailable = available; return *this;};
    UserDataIngSession &includedInCarouselObjectManifest(bool in_manifest) { m_includedInCarouselObjectManifest = in_manifest; return *this;};
    UserDataIngSession &userServiceAnnouncement(const std::shared_ptr<reftools::mbsf::UserServiceDescription> &user_service_description);

    UserDataIngSession &createTimer();
    UserDataIngSession &createCurrentStateTimer();
    bool startTimer();
    const reftools::mbsf::DistSessionState &getDistSessionState(const std::optional<std::shared_ptr<reftools::mbsf::DistSessionState> > &user_state) const;

    void processUserDataIngSessionUpdate(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request, fiveg_mag_reftools::CJson &json);
    void processDistributionSessionInfo(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request);
    void handleUserDataIngSessionUpdate(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request);
    void updateMbstfRemovedDistSession();
    const std::shared_ptr<ServiceScheduleDesc> &findServiceScheduleDesc(const std::string &id) const;
    void addToDistributionSessionInfos(const std::string &key, const std::shared_ptr<ContextData> &context);
    std::shared_ptr< UserDataIngSession::ContextData > getDistributionSessionInfoData(const std::string &key);
    void removeDistributionSessionInfo(const std::string &key);
    void deleteDistributionSessionInfo(const std::string &key);
    void clearDistributionSessionInfos();

    void removeContextData(const std::shared_ptr<ContextData> &context_data);

    void sendMbstfRequests();
    void sendMbstfDelRequests(const std::optional<std::string>& key = std::nullopt);

    void sendMbstfPatchRollbackRequests();

    void sendLocalEvent(OgsExtendedEventId event_id, void *data);
    void sendLocalEventPatch(const std::optional<std::string>& key);
    void sendLocalEventCarouselObjectManifest(const std::optional<std::string>& key);
    bool sendNmbsfMbsUserDataIngestResponse(const std::shared_ptr<UserDataIngDistSessId> &ids);

    bool checkIfAllMBSTFDistSessionDeleted();
    bool checkIfAllMBSSessionCreated();
    bool checkIfAllMBSSessionDeletionsReceived();
    bool checkIfAllMBSTFResponsesReceived();
    bool resetReceivedMBSTFResponseFlags();
    bool checkIfAllMBSTFPatchResponsesReceived();

    bool checkIfAllMBSDistributionSessionsEstablished();
    bool checkIfAllMBSDistributionSessionsTerminated();
    void resetMBSDistributionSessionsTerminatedFlag();
    void resetMBSDistributionSessionsEstablishedFlag();

    bool isMBSSessionCreated(const std::string &key);
    bool hasMbstfResponded(const std::string &key);
    reftools::mbsf::DistSessionState lastReportedState(const std::string &key);
    std::optional<std::string> objectIngestBaseUrl(const std::string &key);
    std::optional<std::string> objectAcquisitionIdPush(const std::string &key);
    std::optional<std::string> getObjectAcquisitionIdPush(const std::shared_ptr<reftools::mbsf::DistSession> &session);
    bool inDesiredState(const std::string &key);

    std::list<std::shared_ptr<DistributionSessionDesc>> distributionSessionDescs();
    std::optional<std::list<std::shared_ptr<ServiceScheduleDesc>>> serviceScheduleDescs();
    std::shared_ptr<UserServiceDesc> userServiceDesc();

    std::optional<SubscribedEvents::DateTime> timeOfLatestDistributionSessionEvent(SubscribedEvents::EventTypeBitMask event_type) const;
    void forEachDistributionSessionInfo(
                        const std::function<bool(const std::string&, const std::shared_ptr<ContextData>&)> &fn);
    void forEachDistributionSessionInfo(
                        const std::function<bool(const std::string&, const std::shared_ptr<const ContextData>&)> &fn) const;

    void forEachServiceScheduleDesc(
                        const std::function<bool(const std::string&, const std::shared_ptr<ServiceScheduleDesc>&)> &fn);
    void forEachServiceScheduleDesc(
                        const std::function<bool(const std::string&, const std::shared_ptr<const ServiceScheduleDesc>&)> &fn) const;

    void serviceScheduleDescsUpdate(const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &mbs_user_data_ing_session);

    static const char *localEventGetName( ogs_event_t *event);

    /** Find AP defined UserDataIngSession by Id
     *
     * @param id The UserDataIngSession id to find
     * @throw std::out_of_range if id does not exist
     */
    static const std::shared_ptr<UserDataIngSession> &find(const std::string &id);

    /** Find UserDataIngSession by Id
     *
     * This includes the internal User Service Announcement Channel UserDataIngSession alongside the AP defined UserDataIngSessions.
     *
     * @param id The UserDataIngSession id to find
     * @throw std::out_of_range if id does not exist
     */
    static const std::shared_ptr<UserDataIngSession> &locate(const std::string &id);

    static std::shared_ptr<ContextData> setDistSessionId(const std::shared_ptr<ContextData> &context_data,
                                                         const std::string &dist_session_id);
    static void setMBSSessionFlag(const UserDataIngDistSessId &ids);
    static void setMBSSessionDeleted(const UserDataIngDistSessId &ids);
    static void setMBSTFDistSessionDeletedFlag(const std::string &dist_session_id);
    static bool processEvent(Open5GSEvent &event);
    static bool handleMbstfDiscover(ogs_sbi_nf_instance_t *nf_instance, ogs_sbi_xact_t *xact);
    static bool createMbsSession(const std::shared_ptr<ContextData> &context_data);

    static bool processDistSession(const std::shared_ptr<reftools::mbsf::DistSession> &dist_session);
    static std::shared_ptr<reftools::mbsf::ObjDistributionOperatingMode> getOperatingMode(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::PktDistributionOperatingMode> getPktDistributionOperatingMode(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::ObjAcquisitionMethod> getAcquisitionMethod(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getObjectIngestUrl(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getObjectDistributionUrl(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getTrafficMarkingInfo(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::shared_ptr<reftools::mbsf::PacketDistrMethInfo> > getPktDistributionInfo(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::PktIngestMethod> getPktIngestMethod(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::MbStfIngestAddr> getMbstfIngestAddr(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::list<std::optional<std::string>, fiveg_mag_reftools::OgsAllocator<std::optional<std::string>>> getObjectAcquisitionIds(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::string maxContBitRate(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static bool tmgi(mb_smf_sc_tmgi_t *tmgi, const UserDataIngDistSessId &ids);
    static std::optional<std::string> getObjectIngestBaseUrl(const std::shared_ptr<reftools::mbsf::DistSession> &session);

    static void removeDistributionSessionInfos(const UserDataIngDistSessId &ids);

    static std::shared_ptr<ContextData> getContextData(const std::shared_ptr<UserDataIngDistSessId> &ids);

    bool checkIfAllMBSSessionResponsesReceived();
    void handleFailedMBSSession();
    void setMbstfsInDesiredState();
    void checkDesiredState();
    void pendingDeleteResponse(ogs_pool_id_t stream_id);
    void pushNotificationsEvent() const;

    bool checkIfAllMBSDistributionSessionsEstablishedOrActive();
    const std::shared_ptr<UserService> &mbsUserService();

    bool distributionSessionInfoHasMbstfandMbsSession(const std::string &key);
    bool userDataIngSessionForServiceAnnChannel(const std::shared_ptr<UserDataIngDistSessId> &ids);
    bool isUserServiceAnnouncementChannel(const std::string &distribution_session_info_key);
    void userServiceAnnChannelDistributionSessionInfo();
    const std::list<std::string> &getUserServiceAnnBundleFilesList() const;
    void setDistSessionState(const std::shared_ptr<reftools::mbsf::DistSessionState> &state);
    void configureUserServiceAnnouncementBundler();
    void userServiceAnnBundled();
    std::shared_ptr<reftools::mbsf::DistSessionState> stateOfDistSession(const std::string &key);
    void addCarouselObject(std::shared_ptr<CarouselObject > carousel_object);
    std::list<std::shared_ptr<CarouselObject >> getCarouselObjects() const;
    void resetCarouselObjects();
    void forEachObject(std::function<void(const std::string &)> fn);
    void addObjectLocator(std::string object_locator);
    std::list<std::string> getObjectLocators() const;
    void resetObjectLocators();
    void userSerAdNotificationSent(bool notification_sent) const;

    ActivePeriodsBase::TimeRange activeTimeRange() const { return m_activePeriods?m_activePeriods->activeTimeRange():ActivePeriodsBase::TimeRange{std::nullopt, std::nullopt}; };

    void requiresUserServiceAnnouncement();

    static void changeDistSessionState(void *data);
    static void currentDistSessionState(const UserDataIngDistSessId &ids);

    static void setMBSSessionFailureFlag(const UserDataIngDistSessId &ids, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt, const std::optional<fiveg_mag_reftools::CJson> &problem_detail_json = std::nullopt);
    static void populateAndSendError(UserDataIngDistSessId *ids, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt,
                    const std::optional<fiveg_mag_reftools::CJson> &problem_detail_json = std::nullopt);
    static void deleteMBSTFSession(ogs_sbi_xact_t *xact);
    static bool handlePatchUpdateResponse(ogs_sbi_xact_t *xact, const std::shared_ptr<reftools::mbsf::DistSession> &dist_session);
    static void rollbackMBSTFDistSessionState(ogs_sbi_xact_t *xact);

    static void sendNotificationsEvent(const std::shared_ptr<UserDataIngDistSessId> &user_data_ing_dist_sess_ids);
    static void sendMbsmfActivityStatus(const std::shared_ptr<UserDataIngDistSessId> &user_data_ing_dist_sess_ids);

    static void addToRegistry(ogs_sbi_xact_t* xact, const std::shared_ptr<UserDataIngDistSessId> &ids);
    static void removeFromRegistry(ogs_sbi_xact_t* xact);
    static std::shared_ptr<UserDataIngDistSessId> getFromRegistry(ogs_sbi_xact_t* xact);

    static void addToRegistry(const std::string &dist_session_id, const std::shared_ptr<UserDataIngDistSessId> &ids);
    static void removeFromRegistry(const std::string &dist_session_id);
    static std::shared_ptr<UserDataIngDistSessId> getFromRegistry(const std::string &dist_session_id);

    static void removeXact(ogs_sbi_xact_t* xact);
    static int numberOfDistributionSessions();

    static void clearRegistries() { std::lock_guard<std::recursive_mutex> lock(s_registry_mutex); s_xactRegistry.clear(); s_distSessionIdRegistry.clear(); };

private:
    void updateContexts(ogs_pool_id_t stream_id, const std::shared_ptr<Open5GSSBIRequest> &request);
    void _changeDistSessionState();
    UserDataIngSession &setUserServiceAnnBundler();
    void populateCarouselObject(const std::shared_ptr<Open5GSSBINFInstance> &nf_instance);
    void populateObjectCarousel(std::set<std::string> &user_serv_ann_server_addrs);

    static std::recursive_mutex s_registry_mutex;
    static std::map<ogs_sbi_xact_t*, std::shared_ptr<UserDataIngDistSessId>> s_xactRegistry;
    static std::map<std::string, std::shared_ptr<UserDataIngDistSessId>> s_distSessionIdRegistry;

    std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> m_MBSUserDataIngSession;
    std::unique_ptr<std::recursive_mutex> m_distSessInfosMutex;
    std::unique_ptr<std::recursive_mutex> m_deleteRequestsMutex;
    std::shared_ptr<std::recursive_mutex> m_serviceScheduleDescMutex;
    std::unique_ptr<Open5GSSBIObject> m_sbiObject;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_UserDataIngSessionId;
    std::shared_ptr<ActivePeriodsBase> m_activePeriods;
    std::unique_ptr<Open5GSTimer> m_activePeriodsTimer;
    bool m_startTimer;
    int32_t m_serviceScheduleDescriptionVersion; // next ver no.
    std::shared_ptr<UserServiceAnnBundle> m_userServiceAnnBundle;
    std::shared_ptr<std::recursive_mutex> m_carouselObjectMutex;
    std::list<std::shared_ptr<CarouselObject >> m_objects;
    std::shared_ptr<std::recursive_mutex> m_objectLocatorMutex;
    std::list<std::string> m_objectLocators;
    bool m_userServiceAnnBundleAvailable;
    bool m_includedInCarouselObjectManifest;
    mutable bool m_userSerAdNotificationSent;
    //std::shared_ptr<ObjManifest> m_carouselObjectManifest;

    //key: Dist Session Infos present in this User Data Ingest Session
    std::map<std::string, std::shared_ptr< ContextData >> m_distributionSessionInfos;

    std::list<ogs_pool_id_t> m_deleteRequests;

    std::map<std::string, std::shared_ptr<ServiceScheduleDesc> > m_serviceScheduleDescs;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_DATA_ING_SESSION_HH_ */
