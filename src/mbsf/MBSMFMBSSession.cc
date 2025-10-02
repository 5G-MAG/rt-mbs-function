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
#include "UserDataIngSession.hh"

#include "MBSMFMBSSession.hh"

MBSF_NAMESPACE_START

MBSMFMBSSession::MBSMFMBSSession()
    :m_session(nullptr)
{
}

MBSMFMBSSession::MBSMFMBSSession(mb_smf_sc_mbs_session_t *session)
    :m_session(session)
{
}

MBSMFMBSSession::~MBSMFMBSSession()
{
    //deleteSession();
}

void MBSMFMBSSession::deleteSession()
{
    mb_smf_sc_mbs_session_delete(m_session);
    mb_smf_sc_mbs_session_push_changes(m_session);	
}

MBSMFMBSSession &MBSMFMBSSession::setSession(mb_smf_sc_mbs_session_t *session)
{
    if(!m_session) {
        m_session = session;
    }
    return *this;
};

bool MBSMFMBSSession::processEvent(Open5GSEvent &MBSMFEvent)
{
    ogs_event_t *event = MBSMFEvent.ogsEvent();
    switch (event->id) {
    case MBSF_LOCAL:
        {
            LocalEvent *mbsf_event = ogs_container_of(event, LocalEvent, event);

            switch (mbsf_event->id) {
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

                                if(sa->ogs_sa_family == AF_INET) {
                                    ogs_debug("Recieved IPv4 tunnel address");
                                } else if(sa->ogs_sa_family == AF_INET6) {
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
                          {
                              char *txt = cJSON_Print(problem);
			  }
                          if(mbsf_event->problem_details->cause) {
                              std::optional<fiveg_mag_reftools::ProblemCause> cause = MBSProblemCause::lookup(std::string(mbsf_event->problem_details->cause));
                              if(cause.has_value()) {
				  {   
				      fiveg_mag_reftools::ProblemCause &cause_val = cause.value();
			              int status_code = cause_val.statusCode();
			              const std::string &reason = cause_val.reason();
			              const std::string &cause_str = cause_val.cause();
				  }
                                  //UserDataIngSession::handleMBSSessionError(event->sbi.data, cause.value(), problem_detail);
				  UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, cause.value(), problem_detail);
                                  return true;
                              }
                         } else {
			    UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR, problem_detail);
                         }
                         return true;
                    }

                    //UserDataIngSession::handleMBSSessionError(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR);
                    UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR);

                } else {
                   UserDataIngSession::setMBSSessionFailureFlag(event->sbi.data, ProblemCause::INBOUND_SERVER_ERROR);
                }
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
    default:
        break;
    }

    return "MBSF_LOCAL_EVENT Unknown";
}

MBSMFMBSSession &MBSMFMBSSession::setServiceType(mb_smf_sc_mbs_service_type_e service_type)
{
    mb_smf_sc_mbs_session_set_service_type(m_session, service_type);
    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setTunnelRequest(bool request_udp_tunnel)
{
    mb_smf_sc_mbs_session_set_tunnel_request(m_session, request_udp_tunnel);
    return *this;
}

void MBSMFMBSSession::pushChanges()
{
    mb_smf_sc_mbs_session_push_changes(m_session);
}

void MBSMFMBSSession::mbsSessionCreatedCallback(mb_smf_sc_mbs_session_t *session, int result, const OpenAPI_problem_details_s*  problem_details, void *data)
{

    /* callback for result of MBS Session create operation */

    /* queue result event */
    sendLocalEvent(MBSF_LOCAL_EVENT_MBS_SESSION_CREATE_RESULT, session, result, problem_details?OpenAPI_problem_details_copy(nullptr, const_cast<OpenAPI_problem_details_s *> (problem_details)):nullptr, data);
}

MBSMFMBSSession &MBSMFMBSSession::setCreatedCallback(void *callback_data)
{
    m_session->create_result_cb = mbsSessionCreatedCallback;
    m_session->create_result_cb_data = (void*)callback_data;

    return *this;
}

MBSMFMBSSession &MBSMFMBSSession::setTmgiRequest(bool req_tmgi)
{
    mb_smf_sc_mbs_session_set_tmgi_request(m_session, req_tmgi);
    return *this;
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

