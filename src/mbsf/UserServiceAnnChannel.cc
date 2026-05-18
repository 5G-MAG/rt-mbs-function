/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: User Service Announcement Channel class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// C library includes
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// standard template library includes
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <iostream>
#include <optional>
#include <uuid/uuid.h>
#include <netinet/in.h>
#include <list>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "Curl.hh"
#include "DistributionSessionInfo.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Nmb2Build.hh"
#include "ObjManifest.hh"
#include "Open5GSEvent.hh"
#include "Open5GSSBIClient.hh"
#include "Open5GSSBINFInstance.hh"
#include "Open5GSSBIObject.hh"
#include "UserDataIngSession.hh"

#include "openapi/model/ObjectManifest.h"
#include "openapi/model/ObjDistributionOperatingMode.h"
#include "openapi/model/ObjAcquisitionMethod.h"

#include "utilities.hh"

#include "UserServiceAnnChannel.hh"

using reftools::mbsf::DistributionMethod;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjectManifest;

MBSF_NAMESPACE_START

namespace {
    struct RequestData {
        ~RequestData() {};
        const UserServiceAnnChannel *channel;
        std::shared_ptr<Open5GSSBIRequest> request;
    };
}


UserServiceAnnChannel::UserServiceAnnChannel()
        :m_userDataIngSessions()
	,m_userServiceAnnChannelDataIngSession(nullptr)
	,m_client(nullptr)
	,m_curl(nullptr)
	,m_pushUrl()
	,m_announcementChannelChange()
	,m_announcementChannelMutex(new std::recursive_mutex)
	,m_announcementChannelCancel(false)

{
    //populate spl m_userServiceAnnChannelDataIngSession
    populateUserDataIngSession();
    startWorker();
};

void  UserServiceAnnChannel::populateUserDataIngSession()
{
    std::string user_data_ing_session_id = USER_SERVICE_ANN_CHANNEL;
    std::string mbs_user_service_id = USER_SERVICE_ANN_CHANNEL;

    const std::shared_ptr<DistributionSessionInfo > distribution_session_info = populateDistributionSessionInfo();

    std::map<std::string, std::shared_ptr< DistributionSessionInfo > > distribution_session_infos;
    distribution_session_infos.emplace(USER_SERVICE_ANN_CHANNEL, distribution_session_info);

    m_userServiceAnnChannelDataIngSession.reset(new UserDataIngSession(user_data_ing_session_id, mbs_user_service_id, distribution_session_infos));

    m_userServiceAnnChannelDataIngSession->userServiceAnnChannelDistributionSessionInfo();


}

const std::shared_ptr<DistributionSessionInfo > UserServiceAnnChannel::populateDistributionSessionInfo()
{
    std::shared_ptr<DistributionSessionInfo> distribution_session_info = nullptr;
    std::shared_ptr<reftools::mbsf::ObjDistributionOperatingMode> operating_mode = nullptr;
    std::shared_ptr< reftools::mbsf::ObjAcquisitionMethod > obj_acquisition_method = nullptr;
    std::shared_ptr< reftools::mbsf::DistributionMethod > distribution_method = nullptr;
    std::shared_ptr< reftools::mbsf::DistSessionState > dist_session_state = nullptr;

    operating_mode.reset(new ObjDistributionOperatingMode());
    *operating_mode = reftools::mbsf::ObjDistributionOperatingMode::VAL_CAROUSEL;

    obj_acquisition_method.reset(new ObjAcquisitionMethod());
    *obj_acquisition_method = reftools::mbsf::ObjAcquisitionMethod::VAL_PUSH;

    reftools::mbsf::ObjectDistrMethInfo::ObjAcqIdsType acq_ids;
    acq_ids.emplace_back("carousel");

    distribution_method.reset(new DistributionMethod());
    *distribution_method = reftools::mbsf::DistributionMethod::VAL_OBJECT;

    const std::string &ssm_source_address = App::self().context()->userServiceAnnSsmSourceAddress();
    const std::string &ssm_destination_address = App::self().context()->userServiceAnnSsmDestinationAddress();
    const std::string &mbr = App::self().context()->userServiceAnnMbr();

    dist_session_state.reset(new DistSessionState());
    *dist_session_state = reftools::mbsf::DistSessionState::VAL_INACTIVE;

    distribution_session_info.reset(new DistributionSessionInfo(operating_mode, obj_acquisition_method, acq_ids, distribution_method,
			   ssm_source_address, ssm_destination_address, mbr, dist_session_state));

    return distribution_session_info;

}

void UserServiceAnnChannel::workerLoop()
{
    m_announcementChannelRunning = true;

    bool mbs_session = false;
    bool mbstf_dist_session = false;
    bool dist_session_active = false;
    bool dist_session_inactive = false;

    while (true) {
            
        bool has_ing_session = false;
	{
            std::lock_guard<decltype(m_announcementChannelMutex)::element_type> lock(*m_announcementChannelMutex);
            {

                if (m_announcementChannelCancel) {
                    m_announcementChannelRunning = false;
                    break;
		}

		if(!mbs_session) {
                    while(true) {
                        if (m_announcementChannelCancel) {
                            m_announcementChannelRunning = false;
                            break;
                         }
                         bool mbs_session_created = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
					 [&]{return m_userServiceAnnChannelDataIngSession->isMBSSessionCreated(USER_SERVICE_ANN_CHANNEL);});

                         if(mbs_session_created) {
                             mbs_session = true;
                             break;
                          } 
		    }
		}
		if(!mbstf_dist_session) {
                    m_userServiceAnnChannelDataIngSession->sendMbstfRequests();
                    
		    while (true) {
                        if (m_announcementChannelCancel) {
                            m_announcementChannelRunning = false;
                            break;
                        }
                        bool has_mbstf_responded = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
					[&]{return m_userServiceAnnChannelDataIngSession->isMBSSessionCreated(USER_SERVICE_ANN_CHANNEL) &&
                        m_userServiceAnnChannelDataIngSession->hasMbstfResponded(USER_SERVICE_ANN_CHANNEL);});
                        if(has_mbstf_responded) {
                            mbstf_dist_session = true;
                            break;
                        }
                    }
                }
	        while (true) {
                    if (m_announcementChannelCancel) {
	                m_announcementChannelRunning = false;
                        break;
                    }

		    std::size_t count = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
                       [&]{return m_announcementChannelCancel || countUserDataIngSessions(m_userDataIngSessions);});

		    if (m_announcementChannelCancel) {
                        break;
                    }

		    if(count) {
                        has_ing_session = true;
                        break;
                    } else if(!dist_session_inactive) {
                        has_ing_session = false;
			std::shared_ptr<DistSessionState> state = nullptr;
                        state.reset(new DistSessionState());
                        *state = DistSessionState::VAL_INACTIVE;
                        m_userServiceAnnChannelDataIngSession->setDistSessionState(state);
                        bool inactive_state = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
                                        [&]{return m_announcementChannelCancel || m_userServiceAnnChannelDataIngSession->stateOfDistSession(USER_SERVICE_ANN_CHANNEL)->getValue() == DistSessionState::VAL_INACTIVE;});

			if (m_announcementChannelCancel) break;

			if(inactive_state) {
                            dist_session_active = false;
                            dist_session_inactive = true;
                            break;
                        }
		    }
                }
		if(has_ing_session && !dist_session_active) {
                    while(true) {

                        if (m_announcementChannelCancel) {
                            m_announcementChannelRunning = false;
                            break;
                        }

                        std::shared_ptr<DistSessionState> state = nullptr;
                        state.reset(new DistSessionState());
                        *state = DistSessionState::VAL_ACTIVE;
			m_userServiceAnnChannelDataIngSession->setDistSessionState(state);
                        bool active_state = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
					[&]{return m_userServiceAnnChannelDataIngSession->stateOfDistSession(USER_SERVICE_ANN_CHANNEL)->getValue() == DistSessionState::VAL_ACTIVE;});
                        if(active_state) {
                            dist_session_active = true;
                            dist_session_inactive = false;
                            break;
                        }
                    }

		}
		/*
		std::size_t counter = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
                       [&]{return App::self().context()->annChannelCount();});
		       */

		while (true) {
                    if (m_announcementChannelCancel) {
                        m_announcementChannelRunning = false;
                        break;
                    }

	            if(count()) {
		        break;
		    }

	            has_ing_session = false;
                    std::shared_ptr<DistSessionState> state = nullptr;
                    state.reset(new DistSessionState());
                    *state = DistSessionState::VAL_INACTIVE;
                    m_userServiceAnnChannelDataIngSession->setDistSessionState(state);
                    bool inactive_state = m_announcementChannelChange.wait_for(*m_announcementChannelMutex, std::chrono::milliseconds(10),
                                        [&]{return m_userServiceAnnChannelDataIngSession->stateOfDistSession(USER_SERVICE_ANN_CHANNEL)->getValue() == DistSessionState::VAL_INACTIVE;});
                    if(inactive_state) {
                        dist_session_active = false;
                        dist_session_inactive = true;
                        break;
                    }
		}

	    }

        }

	if(has_ing_session) sendCarouselRequests();
    }

}

int32_t UserServiceAnnChannel::count()
{
    m_count = App::self().context()->annChannelCount();
    notify();
    return m_count;
}

void UserServiceAnnChannel::startWorker()
{
    if (!m_announcementChannelRunning) {
        if (m_announcementChannelThread.joinable()) m_announcementChannelThread.detach();
        m_announcementChannelThread = std::thread([this] () {workerLoop();});
    }
}

UserServiceAnnChannel &UserServiceAnnChannel::addUserDataIngSession(std::weak_ptr<UserDataIngSession> user_data_ing_session)
{
       {
           std::lock_guard<decltype(m_announcementChannelMutex)::element_type> lock(*m_announcementChannelMutex);
           m_userDataIngSessions.emplace_back(user_data_ing_session);
       }
    notify();
    return *this;
}

std::size_t UserServiceAnnChannel::countUserDataIngSessions(const std::list<std::weak_ptr<UserDataIngSession>> &sessions)
{
    m_userDataIngSessions.remove_if(&UserServiceAnnChannel::isExpired);
    return std::count_if(sessions.begin(),sessions.end(),
		    [](const std::weak_ptr<UserDataIngSession>& w) {
        return !UserServiceAnnChannel::isExpired(w);
	}
    );

}

void UserServiceAnnChannel::sendCarouselRequests()
{
    //std::lock_guard<decltype(m_announcementChannelMutex)::element_type> lock(*m_announcementChannelMutex);
    for (auto it = m_userDataIngSessions.begin(); it != m_userDataIngSessions.end(); )
    {
        std::weak_ptr<UserDataIngSession> session = *it;
        it = m_userDataIngSessions.erase(it);
        sendCarousel(session);
    }
}

void UserServiceAnnChannel::sendCarousel(std::weak_ptr<UserDataIngSession> ing_session)
{
    ogs_debug("UserServiceAnnChannel[%p]: Sending Carousel", this);

    std::optional<std::string> base_url = m_userServiceAnnChannelDataIngSession->objectIngestBaseUrl(USER_SERVICE_ANN_CHANNEL);
    std::optional<std::string> push_id = m_userServiceAnnChannelDataIngSession->objectAcquisitionIdPush(USER_SERVICE_ANN_CHANNEL);

    if(!base_url || base_url.value().empty() || !push_id || push_id.value().empty() ) return;
    ogs_debug("UserServiceAnnChannel[%p]: base URL = %s", this, base_url.value().c_str());

    auto session = ing_session.lock();
    if(!session) return;
    const std::shared_ptr<ObjManifest> carousel_object_manifest = session->carouselObjectManifest();

    if(!carousel_object_manifest) return;

    fiveg_mag_reftools::CJson json = carousel_object_manifest->json(true);
    std::string body = std::string(json.serialise());

    ogs_debug("UserServiceAnnChannel[%p]: SENDING: %s", this, body.c_str());

    long bytesReceived = -1;

    if (!m_curl) {
        m_curl = std::make_shared<Curl>();
    }
    std::chrono::milliseconds timeout(10000);
    m_pushUrl = trim_slashes(base_url.value()) + "/" + push_id.value();
    long post(const std::string& url, std::chrono::milliseconds timeout, const std::optional<std::string> &content, const std::optional<std::string> &content_type);
    bytesReceived = m_curl->post(m_pushUrl, timeout, body, std::string(ANN_OBJ_MANIFEST_MIME_TYPE));
    if (bytesReceived >= 0) {
        ogs_debug("Received %ld bytes of data", bytesReceived);
	auto response_code = m_curl->getResponseCode();
        if (response_code >= 200 && response_code <= 299) {
	    ogs_debug("User Service Ann Channel: Carousel Object Manifest: POST Response code [%d].", response_code);
	} else {
	   ogs_debug("User Service Ann Channel: Carousel Object Manifest: POST Response code [%d] not expected.", response_code);
	}

    } else if (bytesReceived == -1) {
        ogs_error("Request timed out.");
    } else {
        ogs_error("An error occurred while posting the data.");
    }

}

void UserServiceAnnChannel::resetClient()
{
    std::optional<std::string> base_url = m_userServiceAnnChannelDataIngSession->objectIngestBaseUrl(USER_SERVICE_ANN_CHANNEL);
    std::optional<std::string> push_id = m_userServiceAnnChannelDataIngSession->objectAcquisitionIdPush(USER_SERVICE_ANN_CHANNEL);

    if(!base_url || base_url.value().empty() || !push_id || push_id.value().empty() ) return;

    if (!m_pushUrl.empty() && m_client) {
        m_pushUrl = trim_slashes(base_url.value()) + "/" + push_id.value();
        ogs_debug("Client Push url: %s", m_pushUrl.c_str());

        m_client.reset(new Open5GSSBIClient(m_pushUrl));
    }
}

bool UserServiceAnnChannel::processEvent(Open5GSEvent &event)
{
    switch (event.id()) {
    case OGS_EVENT_SBI_CLIENT:
        {
            auto &channel = App::self().context()->userServiceAnnouncementChannel();
            if (channel && channel->processClientResponse(event)) return true;
            return false;
	}
    default:
        return false;
    }
    return false;
}

bool UserServiceAnnChannel::processClientResponse(const Open5GSEvent &event)
{
    switch (event.id()) {
    case OGS_EVENT_SBI_CLIENT:
    {
        const void *raw = event.sbiData();
        if (!raw) {
            break;
        }

        uintptr_t p = reinterpret_cast<uintptr_t>(raw);
        bool looks_like_pointer = (p > 0x1000) && (p % alignof(RequestData) == 0);

        if (looks_like_pointer) {
            RequestData *req_data = reinterpret_cast<RequestData*>(const_cast<void*>(raw));
            if (req_data && req_data->channel == this) {

                ogs_debug("Client carousel request [%p] and channel [%p]", req_data, req_data->channel);
                if (event.sbiState() == OGS_OK) {
                    auto resp = event.sbiResponse(true);
                    ogs_debug("Got %i carousel response to %s", resp.status(), req_data->request->uri());
                } else {
                    ogs_debug("Problem sending Object Manifest(s) to %s", req_data->request->uri());
                }

                req_data->request->setOwner(true);
                req_data->request.reset();
                delete req_data;

                return true;
            } else {
	        ogs_debug("Response is not for the announcement channel");
	    }
            break;
        }


        break;
    }
    default:
        break;
    }
    return false;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
