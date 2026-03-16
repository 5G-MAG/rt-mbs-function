/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MBSMF MBS Session class
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

#include <memory>
#include <stdexcept>

#include "common.hh"
#include "App.hh"
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Nmb2Build.hh"
#include "ServiceInfo.hh"
#include "ExternalServiceArea.hh"
#include "ServiceArea.hh"
#include "AssociatedSessId.hh"
#include "UserDataIngSession.hh"
#include "utilities.hh"
#include "openapi/model/CJson.hh"
#include "openapi/model/ProblemCause.hh"

#include "MBSMFMBSSession.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ProblemCause;
using reftools::mbsf::AssociatedSessionId;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsServiceInfo;

MBSF_NAMESPACE_START

MBSMFMBSSession::MBSMFMBSSession()
    :m_session(nullptr)
    ,m_subscription(nullptr)
{
}

MBSMFMBSSession::MBSMFMBSSession(mb_smf_sc_mbs_session_t *session)
    :m_session(session)
    ,m_subscription(nullptr)
{
    //createStatusSubscription(0, static_cast<mb_smf_sc_mbs_session_event_type_e>(-1), nullptr, time(NULL)+3600, (void *)session);
}

MBSMFMBSSession::~MBSMFMBSSession()
{
    deleteSession();
}

void MBSMFMBSSession::deleteSession()
{
    if (m_session) {
        if (m_subscription) mb_smf_sc_mbs_status_subscription_delete(m_subscription);
        mb_smf_sc_mbs_session_delete(m_session);
        mb_smf_sc_mbs_session_push_changes(m_session);
        m_subscription = nullptr;
        m_session = nullptr;
    }
}

const char *MBSMFMBSSession::tmgi() {
    return mb_smf_sc_tmgi_repr(m_session->tmgi);
};

mb_smf_sc_mbs_service_type_e MBSMFMBSSession::getServiceType() const
{
    if (!m_session) return MBS_SERVICE_TYPE_BROADCAST;
    return m_session->service_type;
}

bool MBSMFMBSSession::getTunnelRequest() const
{
    if (!m_session) return false;
    return m_session->tunnel_req;
}

bool MBSMFMBSSession::getTmgiRequest() const
{
    if (!m_session) return false;
    return m_session->tmgi_req;
}

mb_smf_sc_activity_status_e MBSMFMBSSession::getActivityStatus() const
{
    if (!m_session) return MBS_SESSION_ACTIVITY_STATUS_NONE;
    return m_session->activity_status;
}

bool MBSMFMBSSession::getAnyUeInd() const
{
    if (!m_session) return false;
    return m_session->any_ue_ind;
}

bool MBSMFMBSSession::getLocationDependent() const
{
    if (!m_session) return false;
    return m_session->location_dependent;
}

MBSMFMBSSession &MBSMFMBSSession::setSession(mb_smf_sc_mbs_session_t *session)
{
    if (!m_session) {
        m_session = session;
    }
    return *this;
};

MBSMFMBSSession &MBSMFMBSSession::setSubscription(mb_smf_sc_mbs_status_subscription_t *subscription)
{
    if (!m_subscription) {
        m_subscription = subscription;
    }
    return *this;
};


MBSMFMBSSession &MBSMFMBSSession::setServiceInfo(std::shared_ptr< MbsServiceInfo > mbs_service_info)
{
    if (m_session) {
        ServiceInfo service_info(mbs_service_info);
        m_session->mbs_service_info = service_info.populateServiceInfo();
        ogs_assert(m_session->mbs_service_info);
    }
    return *this;

}

MBSMFMBSSession &MBSMFMBSSession::setServiceArea(std::shared_ptr< MbsServiceArea > mbs_service_area)
{
    if (m_session) {
        ServiceArea service_area(mbs_service_area);
        m_session->mbs_service_area = service_area.populateServiceArea();
    }
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setExternalServiceArea(std::shared_ptr< ExternalMbsServiceArea > ext_mbs_service_area)
{
    if (m_session) {
        ExternalServiceArea external_service_area(ext_mbs_service_area);
        m_session->ext_mbs_service_area = external_service_area.populateExternalServiceArea();
    }
    return *this;
}


MBSMFMBSSession &MBSMFMBSSession::setFsaId(const std::string &mbs_fsa_id) {
    if (m_session) {
        mb_smf_sc_mbs_fsa_id_t *fsa_id = mb_smf_sc_mbs_fsa_id_new();
        fsa_id->id = static_cast<uint32_t>(std::stoul(mbs_fsa_id, nullptr, 16));
        ogs_list_init(&m_session->mbs_fsa_ids);
        ogs_list_add(&m_session->mbs_fsa_ids, fsa_id);
    }
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setAssociatedSessionId(std::shared_ptr< AssociatedSessionId > associated_session_id)
{
    if (m_session) {
        AssociatedSessId id(associated_session_id);
        m_session->associated_session_id = id.populateAssociatedSessionId();
    }
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setLocationDependent(bool location_dependent)
{
    if (m_session) {
        m_session->location_dependent = location_dependent;
    }
    return *this;
}

bool MBSMFMBSSession::processEvent(Open5GSEvent &MBSMFEvent)
{
    ogs_event_t *event = MBSMFEvent.ogsEvent();

    switch (event->id) {
    case MBSF_LOCAL:
        {
            LocalEvent *mbsf_event = ogs_container_of(event, LocalEvent, event);

            ogs_debug("MBSMF Event: %s", MBSMFMBSSession::mbsfLocalGetName(mbsf_event));
            switch (mbsf_event->id) {
            case MBSF_LOCAL_EVENT_MBS_SESSION_NOTIFY:
                MBSMFMBSSession::processMbsSessionNotify(mbsf_event->notification,  event->sbi.data);
                return true;
            case MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:
                if (mbsf_event->result == OGS_OK) {
                    ogs_info("MBS Session %s [%p] created", mb_smf_sc_mbs_session_get_resource_id(mbsf_event->mbs_session),
                             mbsf_event->mbs_session);
                    if (mbsf_event->mbs_session->tmgi_req) {
                        if (mbsf_event->mbs_session->tmgi) {
                            mb_smf_sc_tmgi_t *tmgi = mbsf_event->mbs_session->tmgi;

                            UserDataIngSession::tmgi(tmgi, event->sbi.data);

                        } else {
                            ogs_error("TMGI request failed");
                        }
                    }
                    if (mbsf_event->mbs_session->tunnel_req) {
                        if (mbsf_event->mbs_session->mb_upf_udp_tunnel) {
                            char buf[OGS_ADDRSTRLEN + 1];
                            ogs_sockaddr_t *sa;
                            for (sa = mbsf_event->mbs_session->mb_upf_udp_tunnel; sa; sa = sa->next) {

                                if (sa->ogs_sa_family == AF_INET) {
                                    ogs_debug("Recieved IPv4 tunnel address");
                                } else if (sa->ogs_sa_family == AF_INET6) {
                                    ogs_debug("Recieved IPv6 tunnel address");
                                }

                                OGS_ADDR(sa, buf);
                                ogs_debug("  UDP Tunnel = %s:%u", buf, OGS_PORT(sa));
                            }
                        } else {
                            ogs_error("UDP tunnel request failed");
                        }
                    }
                    UserDataIngSession::setMBSSessionFlag(event->sbi.data);
                } else if (mbsf_event->result == OGS_ERROR) {
                    if (mbsf_event->problem_details) {

                        cJSON *problem = OpenAPI_problem_details_convertToJSON((OpenAPI_problem_details_t*)mbsf_event->problem_details);
                        CJson problem_detail(problem, true);
                        if (mbsf_event->problem_details->cause) {
                            std::optional<fiveg_mag_reftools::ProblemCause> cause =
                                        MBSProblemCause::lookup(std::string(mbsf_event->problem_details->cause));
                            if (cause.has_value()) {
                                UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, cause.value(), problem_detail);
                                return true;
                            }
                        } else {
                            UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR, problem_detail);
                        }
                        return true;
                    }
                    UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR);
                } else {
                   UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR);
                }
                return true;
            case MBSF_LOCAL_EVENT_MBS_SESSION_DELETED:
                UserDataIngSession::setMBSSessionDeleted(event->sbi.data);
                return true;
            default:
                ogs_warn("Unexpected local event: %s", mbsfEventGetName(event));
                return true;
            }
        }
    }
    return false;
}

const char *MBSMFMBSSession::mbsfEventGetName( ogs_event_t *event)
{
    LocalEvent *mbsf_event;

    if (ogs_unlikely(!event)) return "*** No Event ***";

    if (event->id != MBSF_LOCAL) return ogs_event_get_name(event);

    mbsf_event = ogs_container_of(event, LocalEvent, event);

    return mbsfLocalGetName(mbsf_event);
}

const char *MBSMFMBSSession::mbsfLocalGetName(LocalEvent *mbsf_event)
{
    if (ogs_unlikely(!mbsf_event)) return "*** No Event ***";

    switch (mbsf_event->id) {
    case MBSF_LOCAL_EVENT_MBS_SESSION_CREATE:
        return "MBSF_LOCAL_EVENT_MBS_SESSION_CREATE";
    case MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:
        return "MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT";
    case MBSF_LOCAL_EVENT_MBS_SESSION_NOTIFY:
        return "MBSF_LOCAL_EVENT_MBS_SESSION_NOTIFY";
    default:
        break;
    }

    return "MBSF_LOCAL_EVENT Unknown";
}

MBSMFMBSSession &MBSMFMBSSession::setServiceType(mb_smf_sc_mbs_service_type_e service_type)
{
    if (m_session) {
        m_session->service_type = service_type;
    }
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setTunnelRequest(bool request_udp_tunnel)
{
    if (m_session) {
        m_session->tunnel_req = request_udp_tunnel;
    }
    return *this;
}

void MBSMFMBSSession::pushChanges()
{
    mb_smf_sc_mbs_session_push_changes(m_session);
}

void MBSMFMBSSession::mbsSessionCallback(mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_s*  problem_details, void *data)
{

    /* callback for result of MBS Session create operation */

    /* queue result event */
    sendLocalEvent((result != OGS_DONE)?MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:MBSF_LOCAL_EVENT_MBS_SESSION_DELETED,
                   session, result,
                   problem_details?OpenAPI_problem_details_copy(nullptr, const_cast<OpenAPI_problem_details_s*>(problem_details))
                                  :nullptr,
                   data);
}

MBSMFMBSSession &MBSMFMBSSession::setCreatedCallback(void *callback_data)
{
    mb_smf_sc_mbs_session_set_callback(m_session, mbsSessionCallback, callback_data);

    return *this;
}

void MBSMFMBSSession::mbsSessionNotifyCallback(const mb_smf_sc_mbs_status_notification_result_t *notification, void *data)
{
    sendLocalNotifyEvent(MBSF_LOCAL_EVENT_MBS_SESSION_NOTIFY, notification, data);
}

void MBSMFMBSSession::processMbsSessionNotify(const mb_smf_sc_mbs_status_notification_result_t *notification, void *data)
{
    /* callback for  mb-smf service consumer library receiving an MBS Session notification */
    std::string event_time = time_t_to_str(notification->event_time);
    switch (notification->event_type) {
    case MBS_SESSION_EVENT_MBS_REL_TMGI_EXPIRY:
        ogs_info("MBS_REL_TMGI_EXPIRY notification:\n"
                 "    Event time: %s",
                 event_time.c_str());
        break;
    case MBS_SESSION_EVENT_BROADCAST_DELIVERY_STATUS:
        ogs_info("BROADCAST_DELIVERY_STATUS notification:\n"
                 "    Event time: %s\n"
                 "    Status: %s",
                 event_time.c_str(),
                 (notification->broadcast_delivery_status==BROADCAST_DELIVERY_STARTED)?"started":"terminated");
        break;
    case MBS_SESSION_EVENT_INGRESS_TUNNEL_ADD_CHANGE:
    {
        char *tunnels = NULL;
        mb_smf_sc_mbs_status_notification_ingress_tunnel_addr_t *node;
        ogs_list_for_each(&notification->ingress_tunnel_add_change, node) {
            const char *sep = "";
            tunnels = ogs_mstrcatf(tunnels, "\n      ");
            if (node->ipv4) {
                char buf[INET_ADDRSTRLEN];
                tunnels = ogs_mstrcatf(tunnels, "%s:%u", inet_ntop(AF_INET, node->ipv4, buf, sizeof(buf)), node->port);
                sep = ", ";
            }
            if (node->ipv6) {
                char buf[INET6_ADDRSTRLEN];
                tunnels = ogs_mstrcatf(tunnels, "%s[%s]:%u", sep, inet_ntop(AF_INET6, node->ipv6, buf, sizeof(buf)), node->port);
            }
        }
        ogs_info("INGRESS_TUNNEL_ADD_CHANGE notification:\n"
                 "    Event time: %s\n"
                 "    Tunnels:"
                 "%s",
                 event_time.c_str(),
                 tunnels);
        ogs_free(tunnels);
    }
        break;
    default:
        ogs_warn("Unknown MBS notification '%s' received", notification->event_type_name);
        break;
    }
}

MBSMFMBSSession &MBSMFMBSSession::createStatusSubscription(uint16_t area_session_id, mb_smf_sc_mbs_session_event_type_e event_type, const char *correlation_id, time_t expiry_time, void *callback_data)
{
    m_subscription = mb_smf_sc_mbs_status_subscription_new( area_session_id /* area_session_id */, event_type /* event_type_flags */,
                    correlation_id /* correlation_id */, expiry_time /* expiry_time */, mbsSessionNotifyCallback /* notify_cb */, callback_data /* cb_data */);

    mb_smf_sc_mbs_session_add_subscription(m_session, m_subscription);
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setTmgiRequest(bool tmgi_req)
{
    if (m_session) {
        //mb_smf_sc_mbs_session_set_tmgi_request(m_session, req_tmgi);
        m_session->tmgi_req = tmgi_req;
    }
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setActivityStatus(mb_smf_sc_activity_status_e activity_status)
{
    if (m_session) {
        m_session->activity_status = activity_status;
    }
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setAnyUeInd(bool any_ue_ind)
{
    if (m_session) {
        m_session->any_ue_ind = !any_ue_ind;
        ogs_info("ANY UE ID: IN PARAM: %d, IN MBS SESSION STRUCT: %d", any_ue_ind, m_session->any_ue_ind);
    }
    return *this;

}

void MBSMFMBSSession::sendLocalNotifyEvent(LocalEventId event_id, const mb_smf_sc_mbs_status_notification_result_t *notification, void *data)
{
    int rv;

    LocalEvent* event = static_cast<LocalEvent*>(ogs_calloc(1, sizeof(*event)));

    ogs_assert(event);

    event->id = event_id;
    event->event.id = MBSF_LOCAL;
    event->event.sbi.data = data;

    event->mbs_session = nullptr;
    event->problem_details = nullptr;
    event->notification = notification;

    rv = ogs_queue_push(ogs_app()->queue, &event->event);
    if (rv != OGS_OK) {
        ogs_error("Failed to push MBSF local event onto the evet queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}

void MBSMFMBSSession::sendLocalEvent(LocalEventId event_id, mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_s*  problem_details, void *data)
{
    int rv;

    LocalEvent* event = static_cast<LocalEvent*>(ogs_calloc(1, sizeof(*event)));

    ogs_assert(event);

    event->id = event_id;
    event->event.id = MBSF_LOCAL;
    event->event.sbi.data = data;

    event->mbs_session = session;
    event->problem_details = problem_details;
    event->result = result;

    rv = ogs_queue_push(ogs_app()->queue, &event->event);
    if (rv != OGS_OK) {
        ogs_error("Failed to push MBSF local event onto the evet queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

