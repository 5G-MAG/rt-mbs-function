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
#include "DistributionSessionInfo.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Nmb2Build.hh"
#include "Open5GSEvent.hh"

#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/MbsSessionId.h"
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

MBSF_NAMESPACE_START

UserServiceAnnChannel::UserServiceAnnChannel()
        :m_userServiceAnnChannelDataIngSession(nullptr)
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

    std::lock_guard<std::recursive_mutex> lock(*m_announcementChannelMutex);

    bool mbs_session = false;
    bool mbstf_dist_session = false;

    while (true) {
        {
            if (m_announcementChannelCancel) {
		m_announcementChannelRunning = false;
                break;
            }

        }

	if(mbs_session && mbstf_dist_session) continue;

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
    }
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
    m_userDataIngSessions.emplace_back(user_data_ing_session);
    return *this;
}

void UserServiceAnnChannel::processEvent(ogs_event_t *event)
{

}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
