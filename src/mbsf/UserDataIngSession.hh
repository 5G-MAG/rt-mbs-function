#ifndef _MBSF_USER_DATA_ING_SESSION_HH_
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
#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "common.hh"
#include "AlwaysActive.hh"
#include "ActivePeriods.hh"
#include "ActivePeriodsRepRule.hh"
#include "DistributionSessionInfo.hh"


namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class Ssm;
    class DistSession;
    class PacketDistrMethInfo;
}

MBSF_NAMESPACE_START

class Open5GSEvent;
class Open5GSSBIRequest;
class Open5GSSBIObject;
class Open5GSTimer;
class ActivePeriods;
class AlwaysActive;
class MBSMFMBSSession;

class UserDataIngSession {
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
        std::string distSessionId;
        bool MBSTFDistSessionDeleted;
        std::string mbstfNFInstanceId;
        std::string mbstfDistSessionId;
        bool distSessionState;
        mb_smf_sc_tmgi_t *tmgi = nullptr;
        std::string mbstfNotificationUrl;
    };

    UserDataIngSession(fiveg_mag_reftools::CJson &json, bool as_request);
    UserDataIngSession(const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &mbs_user_service);
    UserDataIngSession() = delete;
    UserDataIngSession(UserDataIngSession &&other) = delete;
    UserDataIngSession(const UserDataIngSession &other) = delete;
    UserDataIngSession &operator=(UserDataIngSession &&other) = delete;
    UserDataIngSession &operator=(const UserDataIngSession &other) = delete;

    virtual ~UserDataIngSession();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    enum OgsExtendedEventId : int {
        MBSF_LOCAL_SEND_MBSTF_REQ_BUILD = OGS_MAX_NUM_OF_PROTO_EVENT + 1600,
        MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION,
        MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK,
        MBSF_LOCAL_SEND_MBSTF_PATCH_BUILD
    };

    const std::string &userDataIngSessionId() const { return m_UserDataIngSessionId; };
    const std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> &getMBSUserIngSession() const {return m_MBSUserDataIngSession;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};
    const std::string &distSessionState() const ;

    ogs_sbi_xact_t *nmbstfDiscoverOnly(std::shared_ptr< ContextData > data);
    ogs_sbi_xact_t *nmbstfDiscoverAndSend( std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids, ogs_sbi_build_f build, void *context, void *data);
    UserDataIngSession &setNFInstance(ogs_sbi_service_type_e service_type, ogs_sbi_nf_instance_t *nf_instance);

    UserDataIngSession &alwaysActive() {m_alwaysActive.reset(new AlwaysActive()); return *this;};
    UserDataIngSession &alwaysActive(std::shared_ptr<AlwaysActive> always_active) {m_alwaysActive = always_active; return *this;};
    UserDataIngSession &resetAlwaysActive() {m_alwaysActive.reset(); m_alwaysActive = nullptr; return *this;};

    UserDataIngSession &activePeriods(const ActPeriodsType &act_periods) {m_activePeriods.reset(new ActivePeriods(act_periods)); return *this;};
    UserDataIngSession &activePeriods(std::shared_ptr<ActivePeriods> active_periods) {m_activePeriods = active_periods; return *this;};
    UserDataIngSession &resetActivePeriods() {m_activePeriods.reset(); m_activePeriods = nullptr; return *this;};

    UserDataIngSession &activePeriodsRepRule(const ActPeriodsRepRuleType &act_periods_rep_rule) {m_activePeriodsRepRule.reset(new ActivePeriodsRepRule(act_periods_rep_rule)); return *this;};
    UserDataIngSession &activePeriodsRepRule(std::shared_ptr<ActivePeriodsRepRule> active_periods_rep_rule) {m_activePeriodsRepRule = active_periods_rep_rule; return *this;};
    UserDataIngSession &resetActivePeriodsRepRule() {m_activePeriodsRepRule.reset(); m_activePeriodsRepRule = nullptr; return *this;};


    UserDataIngSession &currentDistSessionState(const std::string &state) {m_currentDistSessionState = state; return *this;};


    UserDataIngSession &createTimer();
    UserDataIngSession &createCurrentStateTimer();
    bool startTimer();
    std::shared_ptr<reftools::mbsf::DistSessionState> getDistSessionState();
    const reftools::mbsf::DistSessionState getNextDistSessionState() const;
    const reftools::mbsf::DistSessionState getdistSessState() const;


    void processUserDataIngSessionUpdate(ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request, fiveg_mag_reftools::CJson &json);
    void processDistributionSessionInfo(ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request);
    void handleUserDataIngSessionUpdate(ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request);
    void updateMbstfRemovedDistSession();
    const std::shared_ptr<UserDataIngSession> &findSessionBySbiObject(const std::shared_ptr<Open5GSSBIObject>& sbi_obj);
    void addToDistributionSessionInfos(const std::string &key, const std::shared_ptr<ContextData> &context);
    std::shared_ptr< UserDataIngSession::ContextData > getDistributionSessionInfoData(const std::string &key);
    void removeDistributionSessionInfo(std::string &key);
    void deleteDistributionSessionInfo(std::string &key);
    void clearDistributionSessionInfos();

    void removeContextData(std::shared_ptr<ContextData> context_data);

    void sendMbstfRequests();
    void sendMbstfDelRequests(const std::optional<std::string>& key = std::nullopt);

    void sendMbstfPatchRollbackRequests();

    void sendLocalEvent(OgsExtendedEventId event_id, void *data);
    void sendLocalEventPatch(const std::optional<std::string>& key);
    bool sendNmbsfMbsUserDataIngestResponse(std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids);

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

    std::map<std::string, std::shared_ptr< ContextData >> &distributionSessionInfos();

    static const char *localEventGetName( ogs_event_t *event);

    static const std::shared_ptr<UserDataIngSession> &find(const std::string &id); // throws std::out_of_range if id does not exist

    static std::shared_ptr< UserDataIngSession::ContextData > setDistSessionId(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::string dist_session_id);
    static void setMBSSessionFlag(void *data);
    static void setMBSSessionDeleted(void *data);
    static void setMBSTFDistSessionDeletedFlag(std::string &dist_session_id);
    static bool processEvent(Open5GSEvent &event);
    static bool handleMbstfDiscover(ogs_sbi_nf_instance_t *nf_instance, ogs_sbi_xact_t *xact);

    static bool processDistSession(std::shared_ptr<reftools::mbsf::DistSession> dist_session);
    static std::shared_ptr<reftools::mbsf::ObjDistributionOperatingMode> getOperatingMode(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::PktDistributionOperatingMode> getPktDistributionOperatingMode(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::ObjAcquisitionMethod> getAcquisitionMethod(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getObjectIngestUrl(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getObjectDistributionUrl(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getTrafficMarkingInfo(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::optional<std::shared_ptr<reftools::mbsf::PacketDistrMethInfo> > getPktDistributionInfo(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::PktIngestMethod> getPktIngestMethod(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::shared_ptr<reftools::mbsf::MbStfIngestAddr> getMbstfIngestAddr(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::list<std::optional<std::string>, fiveg_mag_reftools::OgsAllocator<std::optional<std::string> > > getObjectAcquisitionIds(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static std::string maxContBitRate(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &info);
    static bool tmgi(mb_smf_sc_tmgi_t *tmgi, void *data);

    static void removeDistributionSessionInfos(void *data);

    static std::shared_ptr< ContextData > getContextData(const std::shared_ptr<UserDataIngDistSessId> &ids);

    bool checkIfAllMBSSessionResponsesReceived();
    void handleFailedMBSSession();
    void setMbstfsInDesiredState();
    void checkDesiredState();
    void pendingDeleteResponse(ogs_pool_id_t stream_id);
    void pushNotificationsEvent() const;

    static void changeDistSessionState(void *data);
    static void currentDistSessionState(void *data);

    static void setMBSSessionFailureFlag(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt, const std::optional<fiveg_mag_reftools::CJson> &problem_detail_json = std::nullopt);
    static void handleMBSSessionError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt,
                    const std::optional<fiveg_mag_reftools::CJson> &problem_detail_json = std::nullopt);
    static void populateAndSendError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt,
                    const std::optional<fiveg_mag_reftools::CJson> &problem_detail_json = std::nullopt);
    static void deleteMBSTFSession(ogs_sbi_xact_t *xact);
    static void handlePatchUpdateResponse(ogs_sbi_xact_t *xact);
    static void rollbackMBSTFDistSessionState(ogs_sbi_xact_t *xact);

    static void sendNotificationsEvent(std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > user_data_ing_dist_sess_ids);
    static void sendMbsmfActivityStatus(std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > user_data_ing_dist_sess_ids);

    static void addToRegistry(ogs_sbi_xact_t* xact, std::shared_ptr< UserDataIngDistSessId > &ids);
    static void removeFromRegistry(ogs_sbi_xact_t* xact);
    static std::shared_ptr< UserDataIngDistSessId > getFromRegistry(ogs_sbi_xact_t* xact);

    static void addToRegistry(std::string dist_session_id, std::shared_ptr< UserDataIngDistSessId > &ids);
    static void removeFromRegistry(std::string &dist_session_id);
    static std::shared_ptr< UserDataIngDistSessId > getFromRegistry(std::string &dist_session_id);

    static void removeXact(ogs_sbi_xact_t* xact);
    static int numberOfDistributionSessions();

    static void clearRegistries() { std::lock_guard<std::recursive_mutex> lock(s_registry_mutex); s_xactRegistry.clear(); s_distSessionIdRegistry.clear(); };

private:
    static std::recursive_mutex s_registry_mutex;
    static std::map<ogs_sbi_xact_t *, std::shared_ptr< UserDataIngDistSessId >> s_xactRegistry;
    static std::map<std::string, std::shared_ptr< UserDataIngDistSessId >> s_distSessionIdRegistry;

    std::shared_ptr<reftools::mbsf::MBSUserDataIngSession> m_MBSUserDataIngSession;
    std::unique_ptr<Open5GSSBIObject> m_sbiObject;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_UserDataIngSessionId;
    std::shared_ptr<AlwaysActive> m_alwaysActive;
    std::shared_ptr<ActivePeriods> m_activePeriods;
    std::shared_ptr<ActivePeriodsRepRule> m_activePeriodsRepRule;
    std::unique_ptr<Open5GSTimer> m_activePeriodsTimer;
    reftools::mbsf::DistSessionState m_distSessionState;
    reftools::mbsf::DistSessionState m_currentDistSessionState;
    reftools::mbsf::DistSessionState m_desiredDistSessionState;
    bool m_startTimer;

    //key: Dist Session Infos present in this User Data Ingest Session
    std::map<std::string, std::shared_ptr< ContextData >> m_distributionSessionInfos;
    std::unique_ptr<std::recursive_mutex> m_distSessInfosMutex;

    std::unique_ptr<std::recursive_mutex> m_deleteRequestsMutex;
    std::list<ogs_pool_id_t> m_deleteRequests;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_DATA_ING_SESSION_HH_ */
