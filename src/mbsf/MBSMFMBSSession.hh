#ifndef _MBSF_MBSMF_MBS_SESSION_HH_
#define _MBSF_MBSMF_MBS_SESSION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS: MBSMF MBS Session
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
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
#include "ogs-sbi.h"
#include "mb-smf-service-consumer.h"

#include <memory>
#include <optional>
#include <any>

#include "common.hh"

namespace reftools::mbsf {
    class AssociatedSessionId;
    class ExternalMbsServiceArea;
    class MbsServiceArea;
    class MbsServiceInfo;
}

MBSF_NAMESPACE_START

class Open5GSEvent;

class MBSMFMBSSession {
public:
    MBSMFMBSSession();
    MBSMFMBSSession(mb_smf_sc_mbs_session_t *session);
    MBSMFMBSSession(MBSMFMBSSession &&other) = delete;
    MBSMFMBSSession(const MBSMFMBSSession &other) = delete;
    MBSMFMBSSession &operator=(MBSMFMBSSession &&other) = delete;
    MBSMFMBSSession &operator=(const MBSMFMBSSession &other) = delete;
    virtual ~MBSMFMBSSession();

    enum OgsExtendedEventId : int {
        MBSF_LOCAL = OGS_MAX_NUM_OF_PROTO_EVENT + 1500
    };

    enum LocalEventId : int {
        MBSF_LOCAL_EVENT_NONE = 0,
        MBSF_LOCAL_EVENT_MBS_SESSION_CREATE,
        MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT,
        MBSF_LOCAL_EVENT_MBS_SESSION_DELETED,
        MBSF_LOCAL_EVENT_MBS_SESSION_NOTIFY,
        MBSF_LOCAL_EVENT_MAX
    };

    struct LocalEvent {
        ogs_event_t event;
        LocalEventId id;
        mb_smf_sc_mbs_session_t *mbs_session;
        const OpenAPI_problem_details_s*  problem_details;
        const mb_smf_sc_mbs_status_notification_result_t *notification;
        int result;
    };

    static bool processEvent(Open5GSEvent &event);

    mb_smf_sc_mbs_service_type_e getServiceType() const;
    bool getTunnelRequest() const;
    bool getTmgiRequest() const;
    mb_smf_sc_activity_status_e getActivityStatus() const;
    bool getAnyUeInd() const;
    bool getLocationDependent() const;


    MBSMFMBSSession &setAssociatedSessionId(std::shared_ptr<reftools::mbsf::AssociatedSessionId> associated_session_id);
    MBSMFMBSSession &setSession(mb_smf_sc_mbs_session_t *session);
    MBSMFMBSSession &setSubscription(mb_smf_sc_mbs_status_subscription_t *subscription);
    MBSMFMBSSession &createStatusSubscription(uint16_t area_session_id, mb_smf_sc_mbs_session_event_type_e event_type,
                const char *correlation_id, time_t expiry_time, void *callback_data);
    MBSMFMBSSession &setServiceType(mb_smf_sc_mbs_service_type_e service_type);
    MBSMFMBSSession &setTunnelRequest(bool request_udp_tunnel);
    MBSMFMBSSession &setCreatedCallback(void *callback_data);
    MBSMFMBSSession &setTmgiRequest(bool req_tmgi);
    MBSMFMBSSession &setActivityStatus(mb_smf_sc_activity_status_e activity_status);
    MBSMFMBSSession &setAnyUeInd(bool any_ue_ind);
    MBSMFMBSSession &setServiceInfo(std::shared_ptr<reftools::mbsf::MbsServiceInfo> mbs_service_info);
    MBSMFMBSSession &setExternalServiceArea(std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> ext_mbs_service_area);
    MBSMFMBSSession &setServiceArea(std::shared_ptr<reftools::mbsf::MbsServiceArea> mbs_service_area);
    MBSMFMBSSession &setFsaId(const std::string &mbs_fsa_id);
    MBSMFMBSSession &setLocationDependent(bool location_dependent);

    void deleteSession();

    void pushChanges();

    mb_smf_sc_mbs_session_t *mbsmfMBSSession() const { return m_session; };
    ogs_sockaddr_t *tunnelAddr() const { return m_session?m_session->mb_upf_udp_tunnel:nullptr; };
    mb_smf_sc_ssm_addr_t *ssm() const { return m_session?m_session->ssm:nullptr; };
    const char *tmgi();

    operator bool() const { return !!m_session; };
    static const char *mbsfEventGetName(ogs_event_t *event);
    static const char *mbsfLocalGetName(LocalEvent *mbsf_event);

private:
    static void mbsSessionCallback(mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_s*  problem_details, void *data);
    static void mbsSessionNotifyCallback(const mb_smf_sc_mbs_status_notification_result_t *notification, void *data);
    static void sendLocalEvent(LocalEventId event_id, mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_s*  problem_details, void *data);
    static void sendLocalNotifyEvent(LocalEventId event_id, const mb_smf_sc_mbs_status_notification_result_t *notification, void *data);
    static void processMbsSessionNotify(const mb_smf_sc_mbs_status_notification_result_t *notification, void *data);

    mb_smf_sc_mbs_session_t *m_session;
    mb_smf_sc_mbs_status_subscription_t *m_subscription;

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_MBSMF_MBS_SESSION_HH_ */

