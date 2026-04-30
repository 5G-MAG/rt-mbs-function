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

// SDP Library includes
#include <rtsdp/SDP.hh>

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
#include "QoSReq.hh"
#include "UserDataIngSession.hh"
#include "UserService.hh"
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

LIBRTSDP_NAMESPACE_USING(SDP);
LIBRTSDP_NAMESPACE_USING(ConnectionInformation);
LIBRTSDP_NAMESPACE_USING(MediaDescription);
LIBRTSDP_NAMESPACE_USING(Originator);
LIBRTSDP_NAMESPACE_USING(TimingInformation);

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
    std::lock_guard<decltype(m_userServiceAnnMutex)::element_type> lock(*m_userServiceAnnMutex);

    if (m_userServiceAnnThreadCancel) {
        finish();
        return;
    }

    m_userServiceAnnThreadRunning = true;

    if(!writeAnnouncement()) {
        ogs_error("Unable to write announcement.json to local file system");
    }

    m_userDataIngSession->forEachDistributionSessionInfo([this](const auto &id, const auto &ctx) -> bool {
        writeServiceDescriptionProtocolDoc(ctx);
        return true;
    });

    m_userDataIngSession->userServiceAnnBundled(m_userDataIngSession);

    finish();
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

bool UserServiceAnnBundle::writeServiceDescriptionProtocolDoc(const std::shared_ptr<UserDataIngSession::ContextData> &dist_session_ctx)
{
    std::string err;

    std::filesystem::path root_dir{App::self().context()->userServiceAnnDocRoot()};
    root_dir /= m_userDataIngSession->userDataIngSessionId();
    auto metadata_dir = root_dir / ".metadata";

    std::filesystem::path sdp_filename{dist_session_ctx->distSessionInfoKey + ".sdp"};

    std::string session_name = m_userDataIngSession->mbsUserService()->getMBSUserService()->getServNameDescs().front().value()->getServName().value_or(std::string("-"));

    auto [start_time, end_time] = m_userDataIngSession->activeTimeRange();
    auto timings = TimingInformation::makeTimingInformation(start_time.value_or(TimingInformation::NO_TIMESTAMP),
                                                            end_time.value_or(TimingInformation::NO_TIMESTAMP));

    int family = AF_UNSPEC;
    std::string svc_str;
    std::string ssm_source;
    std::string ssm_dest;
    std::string ssm_proto;

    if (dist_session_ctx->ssm) {
        auto &ssm_source_ipv4 = dist_session_ctx->ssm->getSourceIpAddr()->getIpv4Addr();
        auto &ssm_source_ipv6 = dist_session_ctx->ssm->getSourceIpAddr()->getIpv6Addr();
        if (ssm_source_ipv6) {
            ssm_source = *ssm_source_ipv6.value();
            ssm_dest = *dist_session_ctx->ssm->getDestIpAddr()->getIpv6Addr().value();
            ssm_proto = "IN IP6";
            family = AF_INET6;
        } else if (ssm_source_ipv4) {
            ssm_source = ssm_source_ipv4.value();
            ssm_dest = dist_session_ctx->ssm->getDestIpAddr()->getIpv4Addr().value();
            ssm_proto = "IN IP4";
            family = AF_INET;
        } else {
            ogs_error("Unknown SSM address type while building SDP file %s", sdp_filename.string().c_str());
            return false;
        }
        svc_str = "multicast";
    } else {
        svc_str = "broadcast";
    }
    if (dist_session_ctx->tmgi) {
        uint64_t tmgi_val = std::stoll(dist_session_ctx->tmgi->mbs_service_id, nullptr, 16);
        tmgi_val = (tmgi_val << 24) + (dist_session_ctx->tmgi->plmn.mcc2 << 20) + (dist_session_ctx->tmgi->plmn.mcc1 << 16) +
                    (dist_session_ctx->tmgi->plmn.mnc3 << 12) + (dist_session_ctx->tmgi->plmn.mcc3 << 8) +
                    (dist_session_ctx->tmgi->plmn.mnc2 << 4) + dist_session_ctx->tmgi->plmn.mnc1;;
        svc_str += std::format(" {}", tmgi_val);
    }


    auto &primary_language = m_userDataIngSession->mbsUserService()->mainServiceLanguage();
    auto media = MediaDescription::makeMediaDescription("application", dist_session_ctx->ssm_port, "FLUTE/UDP", "0");
    if (!ssm_dest.empty()) {
        auto conn_info = ConnectionInformation::makeConnectionInformation(ssm_dest, family);
        media->connectionInformationAdd(conn_info);
    }
    auto *bitrate = QoSReq::bitrate(dist_session_ctx->info->getMaxContBitRate());
    if (bitrate) {
        media->bandwidthInformationAdd(*bitrate/1000); // SDP bit rates are in kilobits/s
        delete bitrate;
    }
    if (primary_language) media->mediaAttributeAdd("lang", primary_language.value());
    media->mediaAttributeAdd("FEC", "0");

    if (dist_session_ctx->sdp) {
        // Update existing SDP
        if (dist_session_ctx->sdp->sessionName() != session_name) dist_session_ctx->sdp->sessionName(session_name);
        if (*dist_session_ctx->sdp->timingInformations().front() != *timings) {
            dist_session_ctx->sdp->timingInformationsClear();
            dist_session_ctx->sdp->timingInformationAdd(timings);
        }
        if (*dist_session_ctx->sdp->mediaDescriptions().front() != *media) {
            dist_session_ctx->sdp->mediaDescriptionsClear();
            dist_session_ctx->sdp->mediaDescriptionAdd(media);
        }
    } else {
        // Create new SDP
        auto origin = Originator::makeOriginator(ssm_source);

        dist_session_ctx->sdp = SDP::makeSDP(origin, session_name, timings);

        dist_session_ctx->sdp->mediaDescriptionAdd(media);

        dist_session_ctx->sdp->sessionAttributeAdd("mbs-servicetype", svc_str);
        dist_session_ctx->sdp->sessionAttributeAdd("FEC-declaration", "0 encoding-id=0");
        if (!ssm_source.empty()) {
            dist_session_ctx->sdp->sessionAttributeAdd("source-filter", std::format("incl {} * {}", ssm_proto, ssm_source));
        }
        dist_session_ctx->sdp->sessionAttributeAdd("flute-tsi", std::format("{}", dist_session_ctx->tsi));
    }

    bool rv = writeToFile(root_dir.string(), sdp_filename.string(), std::format("{}", *dist_session_ctx->sdp), err);
    if (rv) {
        rv = writeToFile(metadata_dir.string(), sdp_filename.string(), "Content-Type: application/sdp\r\n", err);
        if (rv) {
            addToServingFiles(sdp_filename.string());
        } else {
            ogs_error("Failed to write metadata for %s, Error: %s", sdp_filename.string().c_str(), err.c_str());
        }
    } else {
        ogs_error("Failed to write %s, Error: %s", sdp_filename.string().c_str(), err.c_str());
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
