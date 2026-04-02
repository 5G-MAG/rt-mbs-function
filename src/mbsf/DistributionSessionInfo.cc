/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Distribution Session Info class
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

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// standard template library includes
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <iostream>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "AvailabilityInfo.hh"
#include "ApplicationServiceDesc.hh"
#include "DistributionSessionInfoSubscription.hh"
#include "DistributionSessionDesc.hh"
#include "ObjRepairParameters.hh"
#include "LocalEvents.hh"
#include "SubscribedEvents.hh"
#include "UserDataIngStatSubsc.hh"
#include "UniqueMBSSessionId.hh"
#include "UserDataIngSession.hh"
#include "utilities.hh"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/DistSessionEventReport.h"
#include "openapi/model/DistSessionEventReportList.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/Event.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/ObjectDistrMethInfo.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/StatusSubscribeReqData.h"
#include "openapi/model/StatusNotifyReqData.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "DistributionSessionInfo.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::DistributionMethod;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::DistSessionEventReportList;
using reftools::mbsf::Event;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::FECConfig;
using reftools::mbsf::ObjectDistrMethInfo;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::StatusSubscribeReqData;
using reftools::mbsf::StatusNotifyReqData;

MBSF_NAMESPACE_START

DistributionSessionInfo::DistributionSessionInfo(CJson &json, bool as_request)
    :m_mbsDistributionSessionInfo(new MBSDistributionSessionInfo(json, as_request))
    ,m_eventSubscriptions()
    ,m_eventTimestamps()
    ,m_statusNotifyReqData(nullptr)
    ,m_mbsDistributionSessionInfoSubscription(nullptr)
    ,m_availabilityInfo(nullptr)
    ,m_mutex()
    ,m_dataIngestSessionEstablished(false)
    ,m_dataIngestSessionTerminated(false)
    ,m_dataIngestSessionActivated(false)
    ,m_dataIngestSessionDeactivated(false)
{
    validate();
}

DistributionSessionInfo::DistributionSessionInfo(const std::shared_ptr<MBSDistributionSessionInfo> &mbs_distribution_session_info)
    :m_mbsDistributionSessionInfo(mbs_distribution_session_info)
    ,m_eventSubscriptions()
    ,m_eventTimestamps()
    ,m_statusNotifyReqData(nullptr)
    ,m_mbsDistributionSessionInfoSubscription(nullptr)
    ,m_availabilityInfo(nullptr)
    ,m_mutex()
    ,m_dataIngestSessionEstablished(false)
    ,m_dataIngestSessionTerminated(false)
    ,m_dataIngestSessionActivated(false)
    ,m_dataIngestSessionDeactivated(false)
{
    validate();
}

DistributionSessionInfo::~DistributionSessionInfo()
{
}

CJson DistributionSessionInfo::json(bool as_request = false) const
{
    return m_mbsDistributionSessionInfo->toJSON(as_request);
}

std::shared_ptr<MBSDistributionSessionInfo> &DistributionSessionInfo::updateMBSDistributionSessionInfo(
    std::shared_ptr<MBSDistributionSessionInfo> new_mbs_dist_session_infos)
{

    // --------------------------------------------------------------------
    // 1. Unrestricted updates
    // --------------------------------------------------------------------
    m_mbsDistributionSessionInfo->setMbsServInfo(std::move(new_mbs_dist_session_infos->getMbsServInfo()));

    m_mbsDistributionSessionInfo->setMbsFSAId(std::move(new_mbs_dist_session_infos->getMbsFSAId()));

    m_mbsDistributionSessionInfo->setTgtServAreas(std::move(new_mbs_dist_session_infos->getTgtServAreas()));

    // --------------------------------------------------------------------
    // 2. Conditional updates – only when the session is INACTIVE
    // --------------------------------------------------------------------
    std::optional<std::shared_ptr< DistSessionState > > dist_session_state = m_mbsDistributionSessionInfo->getMbsDistSessState();

    if (dist_session_state.has_value() && dist_session_state.value()->getValue() == DistSessionState::VAL_INACTIVE) {
        // ----- Max Continuous Bit Rate -----
        m_mbsDistributionSessionInfo->setMaxContBitRate(std::move(new_mbs_dist_session_infos->getMaxContBitRate()));

        // ----- Max Continuous Delay -----
        m_mbsDistributionSessionInfo->setMaxContDelay(std::move(new_mbs_dist_session_infos->getMaxContDelay()));

        // ----- Distribution Method -----
        m_mbsDistributionSessionInfo->setDistrMethod(std::move(new_mbs_dist_session_infos->getDistrMethod()));

        // ----- FEC Config -----
        m_mbsDistributionSessionInfo->setFecConfig(std::move(new_mbs_dist_session_infos->getFecConfig()));

        // ----- Object Distribution Method Info -----
        m_mbsDistributionSessionInfo->setObjDistrInfo(std::move(new_mbs_dist_session_infos->getObjDistrInfo()));

        // ----- Packet Distribution Method Info -----
        m_mbsDistributionSessionInfo->setPckDistrInfo(std::move(new_mbs_dist_session_infos->getPckDistrInfo()));

        // ----- Traffic Marking Info -----
        m_mbsDistributionSessionInfo->setTrafficMarkingInfo(std::move(new_mbs_dist_session_infos->getTrafficMarkingInfo()));

        // ----- External Target Service Areas -----
        m_mbsDistributionSessionInfo->setExtTgtServAreas(std::move(new_mbs_dist_session_infos->getExtTgtServAreas()));

        // ----- Multiplexed Service Flag -----
        m_mbsDistributionSessionInfo->setMultiplexedServFlag(std::move(new_mbs_dist_session_infos->getMultiplexedServFlag()));

        // ----- Restricted Flag -----
        m_mbsDistributionSessionInfo->setRestrictedFlag(std::move(new_mbs_dist_session_infos->getRestrictedFlag()));

        // ----- NR RedCap UE Info -----
        m_mbsDistributionSessionInfo->setNrRedCapUeInfo(std::move(new_mbs_dist_session_infos->getNrRedCapUeInfo()));

        // ----- Associated Session Id -----
        m_mbsDistributionSessionInfo->setAssociatedSessionId(std::move(new_mbs_dist_session_infos->getAssociatedSessionId()));
    }

    m_mbsDistributionSessionInfo->setMbsDistSessState(std::move(new_mbs_dist_session_infos->getMbsDistSessState()));

    return m_mbsDistributionSessionInfo;
}

UniqueMbsSessionId DistributionSessionInfo::getUniqueMbsSessionId() const
{
    const auto &mbs_session_id = getMbsSessionId();
    if (!mbs_session_id) return UniqueMbsSessionId();
    const auto &mbs_service_area = getTgtServAreas();
    const auto &ext_mbs_service_area = getExtTgtServAreas();
    return UniqueMbsSessionId(!!mbs_session_id->getSsm(), mbs_session_id, mbs_service_area, ext_mbs_service_area);
}

std::shared_ptr<MbsSessionId> DistributionSessionInfo::getMbsSessionId() const
{
    if (!m_mbsDistributionSessionInfo) return nullptr;
    const auto &mbs_session_id = m_mbsDistributionSessionInfo->getMbsSessionId();
    if (!mbs_session_id) return nullptr;
    return mbs_session_id.value();
}

std::shared_ptr<reftools::mbsf::MbsServiceArea> DistributionSessionInfo::getTgtServAreas() const
{
    if (!m_mbsDistributionSessionInfo) return nullptr;
    const auto &mbs_service_area = m_mbsDistributionSessionInfo->getTgtServAreas();
    if (!mbs_service_area) return nullptr;
    return mbs_service_area.value();
}

std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> DistributionSessionInfo::getExtTgtServAreas() const
{
    if (!m_mbsDistributionSessionInfo) return nullptr;
    const auto &ext_mbs_service_area = m_mbsDistributionSessionInfo->getExtTgtServAreas();
    if (!ext_mbs_service_area) return nullptr;
    return ext_mbs_service_area.value();
}


void DistributionSessionInfo::addEventSubscription(const std::weak_ptr<UserDataIngStatSubsc> &stat_subscription, std::shared_ptr< reftools::mbsf::Event > event)
{
    if (!m_mbsDistributionSessionInfoSubscription) {
        m_mbsDistributionSessionInfoSubscription.reset(new DistributionSessionInfoSubscription(weak_from_this(), stat_subscription));
    }
    m_mbsDistributionSessionInfoSubscription->setEvents(event);

}

void DistributionSessionInfo::resetEventSubscription()
{
    if (!m_mbsDistributionSessionInfoSubscription) return;
    m_mbsDistributionSessionInfoSubscription->resetEvents();
}

void DistributionSessionInfo::removeEventSubscription()
{
    if (!m_mbsDistributionSessionInfoSubscription) return;
    resetEventSubscription();
    m_mbsDistributionSessionInfoSubscription.reset();

}

void DistributionSessionInfo::registerEvent(std::shared_ptr<DistSessionEventReport> dist_sess_event_report)
{
    m_eventTimestamps.registerEvent(dist_sess_event_report);
    //sendSubscriptionNotifications();

}

bool DistributionSessionInfo::resetDataIngestSessionEstablished() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_dataIngestSessionEstablished = false;
    return m_dataIngestSessionEstablished;
};

bool DistributionSessionInfo::resetDataIngestSessionTerminated() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_dataIngestSessionTerminated = false;
    return m_dataIngestSessionTerminated;
};

void DistributionSessionInfo::registerEvent(SubscribedEvents::EventTypeBitMask event_type)
{
    m_eventTimestamps.registerEvent(event_type);
    sendSubscriptionNotifications();
}

void DistributionSessionInfo::sendSubscriptionNotifications()
{
    if (m_mbsDistributionSessionInfoSubscription) m_mbsDistributionSessionInfoSubscription->pushNotificationsEvent();
}

const DistSessionEventReportList::EventReportListType &DistributionSessionInfo::distributionSessionEventReports() const
{
    std::shared_ptr< DistSessionEventReportList > distribution_session_event_reports = m_statusNotifyReqData->getReportList();
    return distribution_session_event_reports->getEventReportList();
}

DistributionSessionInfo &DistributionSessionInfo::processStatusNotifyReqData(std::shared_ptr<UserDataIngSession> ing_sess, CJson &json, bool as_request)
{
    m_statusNotifyReqData.reset(new StatusNotifyReqData(json, as_request));
    //displayEventReports();
    distributionSessionEventReportsSort();
    //displayEventReports();
    processEvents(ing_sess);
    //std::list<std::optional<std::shared_ptr< DistSessionEventReport > >
    const DistSessionEventReportList::EventReportListType &dist_session_event_reports = distributionSessionEventReports();
    for (const auto &dist_session_event_report : dist_session_event_reports) {
        if (!dist_session_event_report.has_value()) continue;

        std::shared_ptr<DistSessionEventReport> dist_sess_event_report  = dist_session_event_report.value();
        if (!dist_sess_event_report) continue;
        registerEvent(dist_sess_event_report);
    }
    sendSubscriptionNotifications();

    return *this;
}

void DistributionSessionInfo::displayEventReports()
{
 const DistSessionEventReportList::EventReportListType &dist_session_event_reports = distributionSessionEventReports();
     for (const auto &dist_session_event_report : dist_session_event_reports) {
        if (!dist_session_event_report.has_value()) continue;

        std::shared_ptr<DistSessionEventReport> dist_sess_event_report  = dist_session_event_report.value();
        if (!dist_sess_event_report) continue;

        std::shared_ptr< DistSessionEventType > distribution_session_event_type = dist_sess_event_report->getEventType();
        DistSessionEventType dist_session_event_type = *distribution_session_event_type;

        std::optional<std::string> time_stamp = dist_sess_event_report->getTimeStamp();
        if (!time_stamp.has_value()) continue;
        ogs_info("Event:[%s] Timestamp: [%s]", distribution_session_event_type->getString().c_str(), time_stamp.value().c_str());

    }
}

DistributionSessionInfo &DistributionSessionInfo::distributionSessionEventReportsSort()
{

    std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<DistSessionEventReport>> dist_session_event_reports_sorted;
    std::shared_ptr< DistSessionEventReportList > sorted_distribution_session_event_reports = nullptr;
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    //std::shared_ptr< DistSessionEventReportList > distribution_session_event_reports = m_statusNotifyReqData->getReportList();
    //DistSessionEventReportList::EventReportListType dist_session_event_reports = distribution_session_event_reports->getEventReportList();
    const DistSessionEventReportList::EventReportListType &dist_session_event_reports = distributionSessionEventReports();
    for (const auto &dist_session_event_report : dist_session_event_reports) {
        if (!dist_session_event_report.has_value()) continue;
        std::shared_ptr<DistSessionEventReport> dist_sess_event_report  = dist_session_event_report.value();
        if (!dist_sess_event_report) continue;
        std::optional<std::string> time_stamp = dist_sess_event_report->getTimeStamp();
        if (!time_stamp.has_value()) continue;
        std::chrono::system_clock::time_point tp = to_time_point_iso8601(time_stamp.value());
        if (tp == std::chrono::system_clock::time_point{}) {
            ogs_error("Epoch parse failed");
            continue;
        }
        dist_session_event_reports_sorted.emplace(tp, dist_sess_event_report);

    }
    for (const auto &dist_session_event_report_sorted : dist_session_event_reports_sorted) {
        if (!sorted_distribution_session_event_reports){
            sorted_distribution_session_event_reports.reset(new DistSessionEventReportList());
        }
        sorted_distribution_session_event_reports->addEventReportList(std::move(dist_session_event_report_sorted.second));
    }
    //dist_session_event_reports.clearEventReportList()
    m_statusNotifyReqData->setReportList(std::move(sorted_distribution_session_event_reports));

    return *this;
}

DistributionSessionInfo &DistributionSessionInfo::processEvents(std::shared_ptr<UserDataIngSession> ing_sess)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const DistSessionEventReportList::EventReportListType &dist_session_event_reports = distributionSessionEventReports();
    for (const auto &dist_session_event_report : dist_session_event_reports) {
        if (!dist_session_event_report.has_value()) continue;
        std::shared_ptr<DistSessionEventReport> dist_sess_event_report  = dist_session_event_report.value();
        if (!dist_sess_event_report) continue;
        std::shared_ptr< DistSessionEventType > distribution_session_event_type = dist_sess_event_report->getEventType();
        DistSessionEventType dist_session_event_type = *distribution_session_event_type;
        switch (dist_session_event_type) {
        case DistSessionEventType::VAL_SERVICE_MANAGEMENT_FAILURE:
            processServiceManagementFailure(dist_sess_event_report);
            break;
        case DistSessionEventType::VAL_DATA_INGEST_FAILURE:
            processDataIngestFailure(dist_sess_event_report);
            break;
        case DistSessionEventType::VAL_DATA_INGEST_SESSION_ESTABLISHED:
            m_dataIngestSessionEstablished = true;
            contextReportedState(ing_sess, DistSessionState::VAL_ESTABLISHED);
            break;
        case DistSessionEventType::VAL_DATA_INGEST_SESSION_TERMINATED:
            m_dataIngestSessionTerminated = true;
            contextReportedState(ing_sess, DistSessionState::VAL_DEACTIVATING);
            break;
        case DistSessionEventType::VAL_SESSION_DEACTIVATED:
            m_dataIngestSessionDeactivated = true;
            contextReportedState(ing_sess, DistSessionState::VAL_INACTIVE);
            break;
        case DistSessionEventType::VAL_SESSION_ACTIVATED:
            m_dataIngestSessionActivated = true;
            contextReportedState(ing_sess, DistSessionState::VAL_ACTIVE);
            break;
        default:
            continue;
       }
    }

    continueStateTransitions(ing_sess);

    return *this;

}

void DistributionSessionInfo::processDataIngestFailure(std::shared_ptr<DistSessionEventReport> dist_sess_event_report)
{
    std::shared_ptr< DistSessionState > dist_session_state(new DistSessionState());
    *dist_session_state = DistSessionState::VAL_INACTIVE;

    setState(dist_session_state);

    std::shared_ptr< DistSessionEventType > dist_session_event(new DistSessionEventType());
    *dist_session_event = DistSessionEventType::VAL_SESSION_DEACTIVATED;

    std::shared_ptr<DistSessionEventReport> dist_session_event_report(new DistSessionEventReport());
    dist_session_event_report->setEventType(dist_session_event);
    dist_session_event_report->setTimeStamp(dist_sess_event_report->getTimeStamp());
    registerEvent(dist_session_event_report);
}

void DistributionSessionInfo::processServiceManagementFailure(std::shared_ptr<DistSessionEventReport> dist_sess_event_report)
{
    std::shared_ptr< DistSessionState > dist_session_state(new DistSessionState());
    *dist_session_state = DistSessionState::VAL_INACTIVE;

    setState(dist_session_state);

    std::shared_ptr< DistSessionEventType > dist_session_event(new DistSessionEventType());
    *dist_session_event = DistSessionEventType::VAL_SESSION_DEACTIVATED;

    std::shared_ptr<DistSessionEventReport> dist_session_event_report(new DistSessionEventReport());
    dist_session_event_report->setEventType(dist_session_event);
    dist_session_event_report->setTimeStamp(dist_sess_event_report->getTimeStamp());
    registerEvent(dist_session_event_report);
}

void DistributionSessionInfo::setState(std::shared_ptr< DistSessionState > dist_session_state)
{
    m_mbsDistributionSessionInfo->setMbsDistSessState(std::move(dist_session_state));
}

std::shared_ptr<AvailabilityInfo> DistributionSessionInfo::populateAvailabilityInfo()
{
    if (!m_mbsDistributionSessionInfo) return nullptr;

    const std::optional<std::shared_ptr< reftools::mbsf::MbsServiceArea > > &tgt_serv_areas = m_mbsDistributionSessionInfo->getTgtServAreas();
    const  std::optional<std::string > &mbs_fSa_id = m_mbsDistributionSessionInfo->getMbsFSAId();
    if (!tgt_serv_areas.has_value() || !mbs_fSa_id.has_value()) return nullptr;
    std::shared_ptr<AvailabilityInfo> availability_info(new AvailabilityInfo(m_mbsDistributionSessionInfo->getTgtServAreas(), m_mbsDistributionSessionInfo->getMbsFSAId()));
    return availability_info;
}

std::optional<std::list<std::shared_ptr<AvailabilityInfo>>> DistributionSessionInfo::availabilityInfos()
{
    if (!m_mbsDistributionSessionInfo) return std::nullopt;
    std::optional<std::list<std::shared_ptr<AvailabilityInfo>>> availability_infos = std::nullopt;
    std::shared_ptr<AvailabilityInfo> availability_info = populateAvailabilityInfo();
    availability_infos= std::list<std::shared_ptr<AvailabilityInfo>>();
    availability_infos->push_back(availability_info);
    return availability_infos;
}

std::optional<std::list<std::shared_ptr<ApplicationServiceDesc>>> DistributionSessionInfo::applicationServiceDescriptions()
{

    if (!m_mbsDistributionSessionInfo) return std::nullopt;
    std::optional< std::list< std::shared_ptr<ApplicationServiceDesc> > > application_service_descs = std::nullopt;
    std::optional< std::shared_ptr<ObjectDistrMethInfo> > obj_dist_method_info = m_mbsDistributionSessionInfo->getObjDistrInfo();
    if (obj_dist_method_info.has_value()) {
        const std::shared_ptr<ObjectDistrMethInfo> &dist_method_info = obj_dist_method_info.value();

        std::string entry_point_prefix;
        const auto &obj_distr_uri = dist_method_info->getObjDistrUri();
        if (!obj_distr_uri) {
            const auto &obj_ing_uri = dist_method_info->getObjIngUri();
            if (!obj_ing_uri) return std::nullopt;
            entry_point_prefix = trim_slashes(obj_ing_uri.value()) + "/";
        } else {
            entry_point_prefix = trim_slashes(obj_distr_uri.value()) + "/";
        }

        application_service_descs = std::list< std::shared_ptr<ApplicationServiceDesc> >();
        const auto &obj_acq_ids = dist_method_info->getObjAcqIds();
        for (const auto &obj_acq_id : obj_acq_ids) {
            if (obj_acq_id.has_value()) {
                application_service_descs->emplace_back(new ApplicationServiceDesc(entry_point_prefix + obj_acq_id.value()));
            }
        }
        if (application_service_descs.value().empty()) application_service_descs.reset();
    }

    return application_service_descs;
}

std::optional<std::shared_ptr<ObjRepairParameters>> DistributionSessionInfo::populateObjRepairParameters(const std::string &user_data_ing_session_id, const std::string &distribution_session_info_key)
{
    if (!m_mbsDistributionSessionInfo) return std::nullopt;

    if (!App::self().context()->objectRepairParameters.objectRepairBaseLocator.has_value() ||
                    !App::self().context()->objectRepairParameters.backOffParametersOffsetTime ||
                    !App::self().context()->objectRepairParameters.backOffParametersRandomTimePeriod)
    {
        return std::nullopt;
    }

    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = m_mbsDistributionSessionInfo->getObjDistrInfo();
    if (!obj_dist_method_info.has_value()) return std::nullopt;
    if (obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();

        const std::optional<std::string > &obj_distr_uri = dist_method_info->getObjDistrUri();
        if (!obj_distr_uri.has_value()) return std::nullopt;

        //auto obj_repair_parameters = std::make_shared<ObjRepairParameters>(std::optional<std::string>{obj_distr_uri});


        std::shared_ptr<ObjRepairParameters> obj_repair_parameters(new ObjRepairParameters(user_data_ing_session_id, distribution_session_info_key, dist_method_info->getObjDistrUri()));
        return obj_repair_parameters;
    }
    return std::nullopt;

}


std::shared_ptr<DistributionSessionDesc> DistributionSessionInfo::populateDistributionSessionDesc(const std::string &user_data_ing_session_id, const std::string &distribution_session_info_key)
{
    if (!m_mbsDistributionSessionInfo) return nullptr;

    std::string session_description_locator = user_data_ing_session_id + "/"+ distribution_session_info_key + ".sdp";

    std::shared_ptr<DistributionSessionDesc> distribution_session_desc(new DistributionSessionDesc(m_mbsDistributionSessionInfo->getDistrMethod(), session_description_locator, applicationServiceDescriptions(), populateObjRepairParameters(user_data_ing_session_id, distribution_session_info_key), availabilityInfos()));
    return distribution_session_desc;
}

void DistributionSessionInfo::validate() const
{
    if (!m_mbsDistributionSessionInfo) throw std::runtime_error("No MBSDistributionSessionInfo to validate");
    const auto &distr_method = m_mbsDistributionSessionInfo->getDistrMethod();
    switch (distr_method->getValue()) {
    case DistributionMethod::VAL_OBJECT:
        {
            const auto &obj_dist_method_info = m_mbsDistributionSessionInfo->getObjDistrInfo();
            if (!obj_dist_method_info) throw std::runtime_error("Must specify objDistrInfo if distrMethod is OBJECT");
        }
        break;
    case DistributionMethod::VAL_PACKET:
        {
            const auto &pkt_dist_method_info = m_mbsDistributionSessionInfo->getPckDistrInfo();
            if (!pkt_dist_method_info) throw std::runtime_error("Must specify pktDistrInfo if distrMethod is PACKET");
        }
        break;
    default:
        break;
    }
}

void DistributionSessionInfo::contextReportedState(const std::shared_ptr<UserDataIngSession> &ing_sess,
                                                   DistSessionState::Enum state)
{
    auto &dist_session_infos = ing_sess->distributionSessionInfos();
    auto it = std::find_if(dist_session_infos.begin(), dist_session_infos.end(), [this](const std::remove_reference<decltype(dist_session_infos)>::type::value_type &val) -> bool { return val.second->distributionSessionInfo.get() == this; });
    if (it == dist_session_infos.end()) return;
    it->second->last_reported_state = state;
}

void DistributionSessionInfo::continueStateTransitions(const std::shared_ptr<UserDataIngSession> &ing_sess)
{
    auto &dist_session_infos = ing_sess->distributionSessionInfos();
    auto it = std::find_if(dist_session_infos.begin(), dist_session_infos.end(), [this](const std::remove_reference<decltype(dist_session_infos)>::type::value_type &val) -> bool { return val.second->distributionSessionInfo.get() == this; });
    if (it == dist_session_infos.end()) return;
    auto &context = it->second;
    const auto &want_state = context->info->getMbsDistSessState();
    if (want_state) {
        if (context->last_reported_state.getValue() == DistSessionState::VAL_INACTIVE &&
            want_state.value()->getValue() == DistSessionState::VAL_ESTABLISHED) {
            // transitioning through INACTIVE to ESTABLISHED
            setState(want_state.value());
            context->stateUpdate = true;
            ing_sess->sendLocalEventPatch(context->distSessionInfoKey);
        }
    }
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

