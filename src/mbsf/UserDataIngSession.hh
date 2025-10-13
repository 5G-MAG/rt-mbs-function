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

#include <any>
#include <chrono>
#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/MBSUserDataIngSession.h"
#include "common.hh"
#include "ActivePeriods.hh"


namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class MBSUserDataIngSession;
    class Ssm;
    class DistSession;
}

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::DistSession;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::Ssm;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;

using ActPeriodsType = MBSUserDataIngSession::ActPeriodsType;
using ActPeriodsRepRuleType = MBSUserDataIngSession::ActPeriodsRepRuleType;

MBSF_NAMESPACE_START

class Open5GSEvent;
class Open5GSSBIRequest;
class Open5GSSBIObject;
class ActivePeriods;
class MBSMFMBSSession;

class UserDataIngSession {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;
    
    //Pair containing User Data Ing Session Id and key for the Dist Session Info
    using UserDataIngDistSessId = std::pair<std::string, std::string>;
    
    //Pair containing Distribution Session ID sent to the MBSTF and the above UserDataIngDistSessId
    using SessionIdContainer = std::pair<std::string, std::shared_ptr< UserDataIngDistSessId >>;

     enum class MBSSessionState {
        NO = 0,       // no session
        CREATED = 1,  // session successfully created
        FAILED    // session creation failed
    };

    struct ContextData {
        std::string ingSessionId;
        std::string distSessionInfoKey;
        std::shared_ptr<MBSDistributionSessionInfo> info;
        std::shared_ptr<Ssm> ssm;
	std::shared_ptr<Open5GSSBIRequest> request;
        ogs_pool_id_t streamId;
        std::shared_ptr<MBSMFMBSSession> MBSSession;
	MBSSessionState MBSSessionStatus = MBSSessionState::NO;
	std::optional<fiveg_mag_reftools::ProblemCause> mbsmfProblemCause = std::nullopt;
       	std::optional<CJson> mbsmfProblemDetailJson = std::nullopt;
        //bool hasMBSSession;
        bool receivedMBSTFResponse = false;
        bool receivedMBSTFPatchResponse = false;
        bool patchUpdateSucceded = false;
        std::string distSessionId;
	bool MBSTFDistSessionDeleted;
	std::string mbstfNFInstanceId;
	bool distSessionState;
	mb_smf_sc_tmgi_t *tmgi = nullptr;

        /*	
        ~ContextData() {
	    ogs_timer_stop(activePeriodsTimer);
            MBSSession.reset();
        }
	*/
	
    };
    UserDataIngSession(CJson &json, bool as_request);
    UserDataIngSession(const std::shared_ptr<MBSUserDataIngSession> &mbs_user_service);
    UserDataIngSession() = delete;
    UserDataIngSession(UserDataIngSession &&other) = delete;
    UserDataIngSession(const UserDataIngSession &other) = delete;
    UserDataIngSession &operator=(UserDataIngSession &&other) = delete;
    UserDataIngSession &operator=(const UserDataIngSession &other) = delete;

    virtual ~UserDataIngSession();

    CJson json(bool as_request) const;

    enum OgsExtendedEventId : int {
        MBSF_LOCAL_SEND_MBSTF_REQ_BUILD = OGS_MAX_NUM_OF_PROTO_EVENT + 1600,
        MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION,
	MBSF_LOCAL_SEND_MBSTF_PATCH_ROLLBACK
    };

    const std::string &userDataIngSessionId() const { return m_UserDataIngSessionId; };
    const std::shared_ptr<MBSUserDataIngSession> &getMBSUserIngSession() const {return m_MBSUserDataIngSession;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};
    const std::string &distSessionState() const ;

    ogs_sbi_xact_t *nmbstfDiscoverOnly(std::shared_ptr< ContextData > data);
    ogs_sbi_xact_t *nmbstfDiscoverAndSend( std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids, ogs_sbi_build_f build, void *context, void *data);
    UserDataIngSession &setNFInstance(ogs_sbi_service_type_e service_type, ogs_sbi_nf_instance_t *nf_instance);
     
    UserDataIngSession &activePeriods(const ActPeriodsType &act_periods) {m_activePeriods.reset(new ActivePeriods(act_periods)); return *this;};
    UserDataIngSession &activePeriods(std::shared_ptr<ActivePeriods> active_periods) {m_activePeriods = active_periods; return *this;};
    UserDataIngSession &createTimer();
    UserDataIngSession &createCurrentStateTimer();
    bool startTimer();
    std::shared_ptr< DistSessionState > getDistSessionState();

     
    void processDistributionSessionInfo( ogs_pool_id_t stream_id, std::shared_ptr<Open5GSSBIRequest> &request);
    const std::shared_ptr<UserDataIngSession> &findSessionBySbiObject(const std::shared_ptr<Open5GSSBIObject>& sbi_obj);
    void addToDistributionSessionInfos(const std::string &key, const std::shared_ptr< ContextData > context);
    std::shared_ptr< UserDataIngSession::ContextData > getDistributionSessionInfoData(std::string &key);
    void removeDistributionSessionInfo(std::string &key);
    void clearDistributionSessionInfos();
    
    void removeContextData(std::shared_ptr<ContextData> context_data);

    void sendMbstfRequests();
    void sendMbstfDelRequests();
    void sendMbstfPatchRollbackRequests();

    void sendLocalEvent(OgsExtendedEventId event_id, void *data);
    bool sendNmbsfMbsUserDataIngestResponse(std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids);
    
    bool checkIfAllMBSTFDistSessionDeleted();
    bool checkIfAllMBSSessionCreated();
    bool checkIfAllMBSTFResponsesReceived();
    bool resetReceivedMBSTFResponseFlags();
    bool checkIfAllMBSTFPatchResponsesReceived();


    static const char *localEventGetName( ogs_event_t *event);

    static const std::shared_ptr<UserDataIngSession> &find(const std::string &id); // throws std::out_of_range if id does not exist
    
    static void setMBSSessionFlag(void *data);
    static void setMBSTFDistSessionDeletedFlag(std::string &dist_session_id);
    static bool processEvent(Open5GSEvent &event);
    static bool handleMbstfDiscover(ogs_sbi_nf_instance_t *nf_instance, ogs_sbi_xact_t *xact);

    static bool processDistSession(std::shared_ptr< DistSession > dist_session);
    static std::shared_ptr< ObjDistributionOperatingMode > getOperatingMode(std::shared_ptr<MBSDistributionSessionInfo> &info);
    static std::shared_ptr< ObjAcquisitionMethod > getAcquisitionMethod(std::shared_ptr<MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getObjectIngestUrl(std::shared_ptr<MBSDistributionSessionInfo> &info);
    static std::optional<std::string> getObjectDistributionUrl(std::shared_ptr<MBSDistributionSessionInfo> &info);
    static std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > getObjectAcquisitionIds(std::shared_ptr<MBSDistributionSessionInfo> &info);
    static std::string maxContBitRate(std::shared_ptr<MBSDistributionSessionInfo> &info);
    static bool tmgi(mb_smf_sc_tmgi_t *tmgi, void *data);

    static void removeDistributionSessionInfos(void *data);

    static std::shared_ptr< ContextData > getContextData(std::shared_ptr<UserDataIngDistSessId> &ids);

    bool checkIfAllMBSSessionResponsesReceived();
    void handleFailedMBSSession();

    static void changeDistSessionState(void *data);
    static void currentDistSessionState(void *data);

    static void setMBSSessionFailureFlag(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt, const std::optional<CJson> &problem_detail_json = std::nullopt);
    static void handleMBSSessionError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt, 
		    const std::optional<CJson> &problem_detail_json = std::nullopt);
    static void populateAndSendError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt, 
		    const std::optional<CJson> &problem_detail_json = std::nullopt);
    static void deleteMBSTFSession(ogs_sbi_xact_t *xact);
    static void handlePatchUpdateResponse(ogs_sbi_xact_t *xact);
    static void rollbackMBSTFDistSessionState(ogs_sbi_xact_t *xact);


    static void addToRegistry(ogs_sbi_xact_t* xact, std::shared_ptr< UserDataIngDistSessId > &ids);
    static void removeFromRegistry(ogs_sbi_xact_t* xact);
    static std::shared_ptr< UserDataIngDistSessId > getFromRegistry(ogs_sbi_xact_t* xact);

    static void addToRegistry(std::string dist_session_id, std::shared_ptr< UserDataIngDistSessId > &ids);
    static void removeFromRegistry(std::string &dist_session_id);
    static std::shared_ptr< UserDataIngDistSessId > getFromRegistry(std::string &dist_session_id);

    static void removeXact(ogs_sbi_xact_t* xact);
    static int numberOfDistributionSessions();

    static std::recursive_mutex m_mutex;
    static std::map<ogs_sbi_xact_t *, std::shared_ptr< UserDataIngDistSessId >> m_xactRegistry;
    static std::map<std::string, std::shared_ptr< UserDataIngDistSessId >> s_distSessionIdRegistry;

private:
    std::shared_ptr<MBSUserDataIngSession> m_MBSUserDataIngSession;
    std::unique_ptr<Open5GSSBIObject> m_sbiObject;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_UserDataIngSessionId;
    std::recursive_mutex m_rmutex;
    std::shared_ptr<ActivePeriods> m_activePeriods;
    ogs_timer_t *m_activePeriodsTimer;
    DistSessionState m_distSessionState;


    //key: Dist Session Infos present in this User Data Ingest Session
    std::map<std::string, std::shared_ptr< ContextData >> m_distributionSessionInfos;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_DATA_ING_SESSION_HH_ */
