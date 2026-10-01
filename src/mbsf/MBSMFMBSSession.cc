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
#include "MBSPlmnId.hh"
#include "UserDataIngSession.hh"
#include "utilities.hh"
#include "openapi/model/CJson.hh"
#include "openapi/model/ProblemCause.hh"
#include "openapi/model/PlmnId.h"
#include "openapi/model/Tmgi.h"

#include "MBSMFMBSSession.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ProblemCause;
using reftools::mbsf::AssociatedSessionId;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::PlmnId;
using reftools::mbsf::Tmgi;

MBSF_NAMESPACE_START

MBSMFMBSSession::MBSMFMBSSession()
    :m_session(nullptr)
    ,m_subscription(nullptr)
    ,m_afSuppliedTmgi(nullptr)
    ,m_changesInFlight(false)
    ,m_sendUpdates(false)
    ,m_deleteRequested(false)
    ,m_id({"",""})
{
}

MBSMFMBSSession::MBSMFMBSSession(mb_smf_sc_mbs_session_t *session)
    :m_session(session)
    ,m_subscription(nullptr)
    ,m_afSuppliedTmgi(nullptr)
    ,m_changesInFlight(false)
    ,m_sendUpdates(false)
    ,m_deleteRequested(false)
    ,m_id({"",""})
{
    //createStatusSubscription(0, static_cast<mb_smf_sc_mbs_session_event_type_e>(-1), nullptr, time(NULL)+3600, (void *)session);
}

MBSMFMBSSession::~MBSMFMBSSession()
{
    deleteSession();
    if (m_session) mb_smf_sc_mbs_session_set_callback(m_session, nullptr, nullptr);
    if (m_subscription) mb_smf_sc_mbs_status_subscription_set_notification_callback(m_subscription, nullptr, nullptr);
    if (m_afSuppliedTmgi) {
        // Ownership: see setTmgi()'s own comment. The library neither copies an AF-supplied
        // TMGI nor frees it when the session is torn down (mbs-session.c's own
        // _mbs_session_public_clear() only frees session->tmgi "if (session->tmgi_req &&
        // session->tmgi != NULL)", and setTmgi() clears tmgi_req for exactly this case) -- this
        // MBSF allocated m_afSuppliedTmgi and is the only owner responsible for freeing it.
        mb_smf_sc_tmgi_free(m_afSuppliedTmgi);
        m_afSuppliedTmgi = nullptr;
    }
}

void MBSMFMBSSession::deleteSession()
{
    /* Released at most once. The owners of this object call deleteSession() explicitly and then drop
       their reference, and the destructor calls it too, so every teardown through
       UserDataIngSession::clearDistributionSessionInfos() or removeDistributionSessionInfo() reached
       here twice. The second call sent a second DELETE for a session whose first release was still in
       flight, and its completion callback then ran against memory the first had already released, which
       ended the process: "ogs_talloc_size: Expectation `ptr' failed". */
    if (m_deleteRequested.exchange(true)) return;
    if (m_session) {
        ogs_debug("MBSMFMBSSession::deleteSession: this=%p, m_session=%p", this, m_session);
        if (m_subscription) mb_smf_sc_mbs_status_subscription_delete(m_subscription);
        mb_smf_sc_mbs_session_delete(m_session);
        if (mb_smf_sc_mbs_session_push_changes(m_session)) {
            m_changesInFlight = true;
            m_sendUpdates = false;
        }
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

std::shared_ptr<MbsServiceArea> MBSMFMBSSession::getReducedServiceArea() const
{
    if (!m_session || !m_session->red_mbs_service_area) return nullptr;
    return ServiceArea::fromServiceArea(m_session->red_mbs_service_area);
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

/* What an MB-SMF refusal becomes in the MBSF's own answer: a cause and, where the MB-SMF gave one, its
   problem detail.

   A cause matched in the propagation table, or the generic case that at least carries a problem_detail,
   is a better answer than a bare INBOUND_SERVER_ERROR with none, so the bare cause is used only with no
   problem_details at all, or when they cannot be copied.

   A cause with no Nmbsf equivalent (propagationTable maps it to nothing, or does not list it) is itself
   withheld, as the table intends, but the rest of the MB-SMF's account is kept. Discarding all of it left
   the consumer with neither the MB-SMF's status nor its detail, only a fallback text that prints internal
   pointer addresses.

   Used for a create's refusal and for an update's, so both are reported alike. */
static std::pair<fiveg_mag_reftools::ProblemCause, std::optional<CJson>> mbsmfRefusal(int result,
                                                                                      const OpenAPI_problem_details_t *problem_details)
{
    ogs_debug("MB-SMF request failed: result %i, problem_details %s, cause [%s]", result,
              problem_details ? "present" : "absent",
              (problem_details && problem_details->cause) ? problem_details->cause : "");
    if (result != OGS_ERROR || !problem_details) return {ProblemCause::INBOUND_SERVER_ERROR, std::nullopt};

    CJson problem_detail(OpenAPI_problem_details_convertToJSON(const_cast<OpenAPI_problem_details_t*>(problem_details)), true);
    if (!problem_details->cause) return {ProblemCause::INBOUND_SERVER_ERROR, problem_detail};

    std::optional<fiveg_mag_reftools::ProblemCause> cause = MBSProblemCause::lookup(std::string(problem_details->cause));
    if (cause.has_value()) return {cause.value(), problem_detail};

    OpenAPI_problem_details_t *without_cause =
        OpenAPI_problem_details_copy(nullptr, const_cast<OpenAPI_problem_details_t*>(problem_details));
    if (!without_cause) return {ProblemCause::INBOUND_SERVER_ERROR, std::nullopt};
    ogs_free(without_cause->cause);
    without_cause->cause = nullptr;
    CJson detail_only(OpenAPI_problem_details_convertToJSON(without_cause), true);
    OpenAPI_problem_details_free(without_cause);
    return {ProblemCause::INBOUND_SERVER_ERROR, detail_only};
}

bool MBSMFMBSSession::processEvent(Open5GSEvent &MBSMFEvent)
{
    ogs_event_t *event = MBSMFEvent.ogsEvent();

    switch (event->id) {
    case MBSF_LOCAL:
        {
            LocalEvent *mbsf_event = ogs_container_of(event, LocalEvent, event);

            ogs_debug("MBSMF Event: %s", MBSMFMBSSession::mbsfLocalGetName(mbsf_event));
            UserDataIngDistSessId *ids = nullptr;
            switch (mbsf_event->id) {
            case MBSF_LOCAL_EVENT_MBS_SESSION_NOTIFY:
                MBSMFMBSSession::processMbsSessionNotify(mbsf_event->notification,  event->sbi.data);
                break;
            case MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:
                {
                    ids = reinterpret_cast<UserDataIngDistSessId*>(event->sbi.data);
                    if (mbsf_event->result == OGS_OK) {
                        ogs_info("MBS Session %s [%p] created", mb_smf_sc_mbs_session_get_resource_id(mbsf_event->mbs_session),
                                 mbsf_event->mbs_session);
                        if (mbsf_event->mbs_session->tmgi_req) {
                            if (mbsf_event->mbs_session->tmgi) {
                                mb_smf_sc_tmgi_t *tmgi = mbsf_event->mbs_session->tmgi;

                                UserDataIngSession::tmgi(tmgi, *ids);
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
                                        ogs_debug("Received IPv4 tunnel address");
                                    } else if (sa->ogs_sa_family == AF_INET6) {
                                        ogs_debug("Received IPv6 tunnel address");
                                    }

                                    OGS_ADDR(sa, buf);
                                    ogs_debug("  UDP Tunnel = %s:%u", buf, OGS_PORT(sa));
                                }
                            } else {
                                ogs_error("UDP tunnel request failed");
                            }
                        }
                        UserDataIngSession::setMBSSessionFlag(*ids);
                    } else {
                        // A cause matched in the propagation table, or the MB-SMF's detail with a cause this API
                        // does not define removed, or the generic INBOUND_SERVER_ERROR where nothing more is known;
                        // mbsmfRefusal() gives the reasoning.
                        const auto refusal = mbsmfRefusal(mbsf_event->result, mbsf_event->problem_details);
                        UserDataIngSession::setMBSSessionFailureFlag(*ids, refusal.first, refusal.second);
                    }
                }
                break;
            case MBSF_LOCAL_EVENT_MBS_SESSION_DELETED:
                ids = reinterpret_cast<UserDataIngDistSessId*>(event->sbi.data);
                UserDataIngSession::setMBSSessionDeleted(*ids);
                break;
            case MBSF_LOCAL_EVENT_MBS_SESSION_UPDATE_RESULT:
                ids = reinterpret_cast<UserDataIngDistSessId*>(event->sbi.data);
                if (mbsf_event->result == OGS_OK) {
                    UserDataIngSession::setMBSSessionUpdateResult(*ids, std::nullopt, std::nullopt);
                } else {
                    const auto refusal = mbsmfRefusal(mbsf_event->result, mbsf_event->problem_details);
                    UserDataIngSession::setMBSSessionUpdateResult(*ids, refusal.first, refusal.second);
                }
                break;
            default:
                ogs_warn("Unexpected local event: %s", mbsfEventGetName(event));
                break;
            }
            if (ids) delete ids;
            if (mbsf_event && mbsf_event->problem_details) {
                OpenAPI_problem_details_free(mbsf_event->problem_details);
                mbsf_event->problem_details = nullptr;
            }
            return true;
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
    case MBSF_LOCAL_EVENT_MBS_SESSION_UPDATE_RESULT:
        return "MBSF_LOCAL_EVENT_MBS_SESSION_UPDATE_RESULT";
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

bool MBSMFMBSSession::pushChanges()
{
    if (m_changesInFlight) {
        m_sendUpdates = true;
        ogs_debug("Delaying pushing changes to MB-SMF");
        return true;
    }
    if (mb_smf_sc_mbs_session_push_changes(m_session)) {
        m_changesInFlight = true;
        m_sendUpdates = false;
        return true;
    }
    return false;
}

void MBSMFMBSSession::mbsSessionCallback(mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_t *problem_details, void *data)
{
    MBSMFMBSSession *mbs_session = reinterpret_cast<MBSMFMBSSession*>(data);

    /* callback for result of MBS Session create operation */
    mbs_session->m_changesInFlight = false;

    ogs_debug("MB-SMF result callback (%i)", result);

    if (result == OGS_DONE) {
        /* SMF session has gone, stop anything else using it */
        ogs_debug("MBSMFMBSSession::mbsSessionCallback: session %p deleted", mbs_session->m_session);
        mbs_session->m_session = nullptr;
        mbs_session->m_subscription = nullptr;
    }

    /* queue result event */
    sendLocalEvent((result != OGS_DONE)?MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT:MBSF_LOCAL_EVENT_MBS_SESSION_DELETED,
                   session, result, problem_details, mbs_session->m_id);

    /* if we have pending changes, try to send them */
    if (mbs_session->m_sendUpdates) {
        mbs_session->m_sendUpdates = false;
        /* Queued changes that turn out to need no request would otherwise never be answered, and an
           update waiting on their result would wait for ever: reported as applied. */
        if (result != OGS_DONE && !mbs_session->pushChanges()) {
            sendLocalEvent(MBSF_LOCAL_EVENT_MBS_SESSION_UPDATE_RESULT, session, OGS_OK, nullptr, mbs_session->m_id);
        }
    }
}

void MBSMFMBSSession::mbsSessionUpdateCallback(mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_t *problem_details, void *data)
{
    MBSMFMBSSession *mbs_session = reinterpret_cast<MBSMFMBSSession*>(data);

    mbs_session->m_changesInFlight = false;

    ogs_debug("MB-SMF update result callback (%i)", result);

    sendLocalEvent(MBSF_LOCAL_EVENT_MBS_SESSION_UPDATE_RESULT, session, result, problem_details, mbs_session->m_id);

    if (mbs_session->m_sendUpdates) {
        mbs_session->m_sendUpdates = false;
        if (!mbs_session->pushChanges()) {
            sendLocalEvent(MBSF_LOCAL_EVENT_MBS_SESSION_UPDATE_RESULT, session, OGS_OK, nullptr, mbs_session->m_id);
        }
    }
}

MBSMFMBSSession &MBSMFMBSSession::setCallback(const UserDataIngDistSessId &dist_sess_id)
{
    m_id = dist_sess_id;
    mb_smf_sc_mbs_session_set_callback(m_session, mbsSessionCallback, reinterpret_cast<void*>(this));
    /* An update's result is not a create's. Reported through the create path, a refused update marked
       the MBS Session as failed to be created, and the Distribution Session was taken down with it. */
    mb_smf_sc_mbs_session_set_update_callback(m_session, mbsSessionUpdateCallback, reinterpret_cast<void*>(this));

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

MBSMFMBSSession &MBSMFMBSSession::setTmgi(std::shared_ptr<Tmgi> tmgi)
{
    if (m_session && tmgi) {
        if (m_afSuppliedTmgi) {
            // Replacing an AF-supplied TMGI this object already built and attached, before it
            // was ever pushed to the MB-SMF. mb_smf_sc_mbs_session_set_tmgi() below only
            // overwrites the session's own tmgi pointer, it does not free what it replaces
            // (mbs-session.c's own _mbs_session_set_tmgi(), code-derived) -- free the stale one
            // now rather than leak it.
            mb_smf_sc_tmgi_free(m_afSuppliedTmgi);
            m_afSuppliedTmgi = nullptr;
        }

        m_afSuppliedTmgi = mb_smf_sc_tmgi_new();
        mb_smf_sc_tmgi_set_mbs_service_id(m_afSuppliedTmgi, tmgi->getMbsServiceId().c_str());
        const std::shared_ptr<PlmnId> &plmn_id = tmgi->getPlmnId();
        if (plmn_id) {
            MBSPlmnId mbs_plmn_id(plmn_id);
            mb_smf_sc_tmgi_set_plmn(m_afSuppliedTmgi, mbs_plmn_id.mcc(), mbs_plmn_id.mnc());
        }

        // Ownership, resolved by reading mb-smf-service-consumer's own code rather than
        // assuming it (code-derived, mbs-session.c): mb_smf_sc_mbs_session_set_tmgi() stores
        // the pointer given to it directly ("session->session.tmgi =
        // _priv_tmgi_to_public(tmgi);" in _mbs_session_set_tmgi()) -- it does not copy the
        // mb_smf_sc_tmgi_t. It also clears the session's own tmgi_req flag as a side effect
        // (mbs-session.h's own documented contract for this function: "the ... TMGI request
        // flag and setting a TMGI are mutually exclusive"), and _mbs_session_public_clear()
        // -- run when the session is eventually torn down -- only calls _tmgi_free() "if
        // (session->tmgi_req && session->tmgi != NULL)", i.e. only for a TMGI the library
        // itself allocated via the TMGI-request path. An AF-supplied TMGI set here is
        // therefore never freed by the library at any point; m_afSuppliedTmgi tracks it so
        // this class's own destructor can free it instead.
        mb_smf_sc_mbs_session_set_tmgi(m_session, m_afSuppliedTmgi);
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

void MBSMFMBSSession::sendLocalEvent(LocalEventId event_id, mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_t *problem_details, const UserDataIngDistSessId &ids)
{
    int rv;

    LocalEvent* event = static_cast<LocalEvent*>(ogs_calloc(1, sizeof(*event)));

    ogs_assert(event);

    event->id = event_id;
    event->event.id = MBSF_LOCAL;
    event->event.sbi.data = new UserDataIngDistSessId(ids);

    event->mbs_session = session;
    if (problem_details) {
        event->problem_details = OpenAPI_problem_details_copy(nullptr, const_cast<OpenAPI_problem_details_t*>(problem_details));
    }
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

