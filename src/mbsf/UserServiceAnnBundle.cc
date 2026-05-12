/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: User Service Announcement class
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
#include <fstream>
#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
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
#include "UserDataIngSession.hh"
#include "UserServiceDesc.hh"

#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/ObjDistributionOperatingMode.h"
#include "openapi/model/ObjAcquisitionMethod.h"

#include "utilities.hh"

#include "UserServiceAnnBundle.hh"

using reftools::mbsf::DistributionMethod;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;

MBSF_NAMESPACE_START

UserServiceAnnBundle::UserServiceAnnBundle(std::shared_ptr<UserDataIngSession> user_data_ing_session)
        :m_userDataIngSession(user_data_ing_session)
        ,m_nameOfFilesToServe()
        ,m_userServiceAnnChange()
        ,m_userServiceAnnMutex(new std::recursive_mutex)
        ,m_userServiceAnnThreadCancel(false)
	,m_userServiceAnnThreadRunning(false)
	,m_done(false)

{
    startWorker();
};

void UserServiceAnnBundle::wait() {
    std::unique_lock<std::recursive_mutex> lock(*m_userServiceAnnMutex);
    m_userServiceAnnChange.wait(lock, [this] { return m_done.load(); });
}

void UserServiceAnnBundle::worker()
{
    {
        std::lock_guard<decltype(m_userServiceAnnMutex)::element_type> lock(*m_userServiceAnnMutex);

        if (m_userServiceAnnThreadCancel) {
            finish();
            return;
        }
        m_userServiceAnnThreadRunning = true;

        if(!writeAnnouncement()) {
           ogs_error("Unable to write announcement.json to local file system");
        }
        //m_userServiceAnnMutex->unlock();
        m_userDataIngSession->userServiceAnnBundled(m_userDataIngSession);

        //m_userServiceAnnMutex->lock();
        //m_userServiceAnnThreadRunning = false;
        finish();
    }

}

void UserServiceAnnBundle::finish() {
    m_done = true;
    notify();
    m_userServiceAnnThreadRunning = false;
}
void UserServiceAnnBundle::startWorker()
{
    if (!m_userServiceAnnThreadRunning) {
        if (m_userServiceAnnThread.joinable()) m_userServiceAnnThread.detach();
        m_userServiceAnnThread = std::thread([this] () {worker();});
    }
}

bool UserServiceAnnBundle::writeAnnouncement()
{
    std::shared_ptr<UserServiceDesc> user_service_desc = m_userDataIngSession->userServiceDesc();
    //const std::shared_ptr<reftools::mbsf::UserServiceDescription> &user_service_description = user_service_desc->userServiceDescription();
    fiveg_mag_reftools::CJson json = user_service_desc->json(true);
    std::string json_str(json.serialise());

    std::filesystem::path abs_directory_path{App::self().context()->userServiceAnnDocRoot()};
    try {
        abs_directory_path = std::filesystem::absolute(abs_directory_path);
    } catch (std::filesystem::filesystem_error &ex) {
        ogs_error("Failed to write announcement.json, Error: %s", ex.what());
        return false;
    }
    abs_directory_path /= m_userDataIngSession->userDataIngSessionId();
    std::filesystem::path metadata_dir = abs_directory_path / ".metadata" / "";
    abs_directory_path /= "";
    std::string user_service_announcement_file_name = "announcement.json";
    std::string err;
    bool rv = writeToFile(abs_directory_path.string(), user_service_announcement_file_name, json_str, err);
    if (rv) {
        rv = writeToFile(metadata_dir.string(), user_service_announcement_file_name, "Content-Type: application/3gpp-mbs-user-service-descriptions+json;version=\"Rel17\"\r\n", err);
        if (rv) {
            addToServingFiles(user_service_announcement_file_name);
        } else {
            ogs_error("Failed to write metadata for %s, Error: %s", user_service_announcement_file_name.c_str(), err.c_str());
        }
    } else {
        ogs_error("Failed to write %s, Error: %s", user_service_announcement_file_name.c_str(), err.c_str());
    }
    return rv;

}

bool UserServiceAnnBundle::writeToFile(const std::string &abs_directory_path, const std::string &file_name,
                const std::string &content, std::string &err)
{
    err.clear();

    try {
        std::filesystem::path directory(abs_directory_path);
        if (!directory.is_absolute()) {
            err = "Provided directory path is not absolute.";
            return false;
        }

        std::error_code ec;
        if (!std::filesystem::exists(directory, ec)) {
            if (!std::filesystem::create_directories(directory, ec)) {
                err = "Failed to create directory: " + ec.message();
                return false;
            }
        } else if (ec) {
            err = "Filesystem error checking directory: " + ec.message();
            return false;
        }

        std::filesystem::path target = directory / file_name;
        std::filesystem::path tmp = target;
        tmp += ".tmp";

        std::ofstream ofs(tmp, std::ios::binary);
        if (!ofs) {
            err = "Failed to open temporary file for writing.";
            return false;
        }

        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!ofs) {
            err = "Write failed.";
            ofs.close();
            std::filesystem::remove(tmp, ec);
            return false;
        }
        ofs.close();

        std::filesystem::rename(tmp, target, ec);
        if (ec) {
            std::filesystem::remove(tmp, ec);
            err = "Failed to move temporary file into place: " + ec.message();
            return false;
        }

        return true;
    } catch (const std::filesystem::filesystem_error &e) {
        err = std::string("Filesystem exception: ") + e.what();
        return false;
    } catch (const std::exception &e) {
        err = std::string("Exception: ") + e.what();
        return false;
    } catch (...) {
        err = "Unknown error.";
        return false;
    }
}

void UserServiceAnnBundle::processEvent(ogs_event_t *event)
{

}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
