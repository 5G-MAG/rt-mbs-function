#ifndef _MBSF_DISTRIBUTION_SESSION_INFO_HH_
#define _MBSF_DISTRIBUTION_SESSION_INFO_HH_
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <chrono>
#include <memory>

#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/StatusNotifyReqData.h"
#include "openapi/model/DistSessionState.h"
#include "common.hh"

#include "DistributionSessionInfoSubscription.hh"
#include "SubscribedEvents.hh"

namespace reftools::mbsf {
    class DistSessionEventReportList;
    class EventNotification;
    class MbsServiceArea;
    class ExternalMbsServiceArea;
}

MBSF_NAMESPACE_START

class AvailabilityInfo;
class ApplicationServiceDesc;
class DistributionSessionDesc;
class DistributionSessionInfoSubscriptions;
class ObjRepairParameters;
class ServiceInfo;
class SubscribedEvents;
class UserDataIngSession;
class UniqueMbsSessionId;

class DistributionSessionInfo : public std::enable_shared_from_this<DistributionSessionInfo> {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    DistributionSessionInfo(fiveg_mag_reftools::CJson &json, bool as_request);
    DistributionSessionInfo(const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &mbs_distribution_session_info);

    DistributionSessionInfo(
        std::shared_ptr<reftools::mbsf::ObjDistributionOperatingMode> operating_mode,
        std::shared_ptr< reftools::mbsf::ObjAcquisitionMethod > obj_acquisition_method,
        const reftools::mbsf::ObjectDistrMethInfo::ObjAcqIdsType &acq_ids,
        const std::shared_ptr< reftools::mbsf::DistributionMethod > distribution_method,
        const std::string &ssm_source_address, const std::string &ssm_destination_address,
        const std::string &mbr,
        std::optional<std::shared_ptr< reftools::mbsf::DistSessionState > > dist_session_state = std::nullopt);

    DistributionSessionInfo() = delete;
    DistributionSessionInfo(DistributionSessionInfo &&other) = delete;
    DistributionSessionInfo(const DistributionSessionInfo &other) = delete;
    DistributionSessionInfo &operator=(DistributionSessionInfo &&other) = delete;
    DistributionSessionInfo &operator=(const DistributionSessionInfo &other) = delete;

    virtual ~DistributionSessionInfo();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &getMBSDistributionSessionInfo() const {return m_mbsDistributionSessionInfo;};
    std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> &updateMBSDistributionSessionInfo(std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> new_mbs_dist_session_infos);
    const std::shared_ptr<reftools::mbsf::DistributionMethod> &getDistributionMethod() const { return m_mbsDistributionSessionInfo->getDistrMethod();};
    const std::optional<std::shared_ptr< reftools::mbsf::DistSessionState > > &distSessionState() const {return m_mbsDistributionSessionInfo->getMbsDistSessState();};
    UniqueMbsSessionId getUniqueMbsSessionId() const;
    std::shared_ptr<reftools::mbsf::MbsSessionId> getMbsSessionId() const;
    std::shared_ptr<reftools::mbsf::MbsServiceArea> getTgtServAreas() const;
    std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> getExtTgtServAreas() const;

    DistributionSessionInfo &setDistSessionState(std::optional<std::shared_ptr< reftools::mbsf::DistSessionState > > state = std::nullopt)  {m_mbsDistributionSessionInfo->setMbsDistSessState(state); return *this;};

    DistributionSessionInfo &processStatusNotifyReqData(std::shared_ptr<UserDataIngSession> ing_sess, fiveg_mag_reftools::CJson &json, bool as_request);
    void addEventSubscription(const std::weak_ptr<UserDataIngStatSubsc> &stat_subscription, std::shared_ptr< reftools::mbsf::Event > event);
    void resetEventSubscription();
    void removeEventSubscription();
    void processDataIngestFailure(std::shared_ptr<DistSessionEventReport> dist_sess_event_report);
    void processServiceManagementFailure(std::shared_ptr<DistSessionEventReport> dist_sess_event_report);

    DistributionSessionInfo &distributionSessionEventReportsSort();
    void displayEventReports();


    DistributionSessionInfo &processEvents(std::shared_ptr<UserDataIngSession> ing_sess);
    bool resetDataIngestSessionEstablished();
    bool resetDataIngestSessionTerminated();

    const reftools::mbsf::DistSessionEventReportList::EventReportListType &distributionSessionEventReports() const;
    const SubscribedEvents &eventTimestamps() const { return m_eventTimestamps; };
    const bool dataIngestSessionEstablished() const { return m_dataIngestSessionEstablished; };
    const bool dataIngestSessionTerminated() const { return m_dataIngestSessionTerminated; };

    std::shared_ptr<AvailabilityInfo> populateAvailabilityInfo();
    std::optional<std::list<std::shared_ptr<AvailabilityInfo>>> availabilityInfos();
    std::optional<std::list<std::shared_ptr<ApplicationServiceDesc>>> applicationServiceDescriptions();
    std::shared_ptr<DistributionSessionDesc> populateDistributionSessionDesc(const std::string &user_data_ing_session_id, const std::string &distribution_session_info_key);
    std::optional<std::shared_ptr<ObjRepairParameters>> populateObjRepairParameters(const std::string &user_data_ing_session_id, const std::string &distribution_session_info_key);
    std::optional<std::string> objectAcqIdsContentType(const std::string &url);

private:
    void setState(std::shared_ptr< reftools::mbsf::DistSessionState > dist_session_state);
    void registerEvent(std::shared_ptr<reftools::mbsf::DistSessionEventReport> dist_sess_event_report);
    void registerEvent(SubscribedEvents::EventTypeBitMask event_type);
    void sendSubscriptionNotifications();
    void validate() const;
    void contextReportedState(const std::shared_ptr<UserDataIngSession> &ing_sess, reftools::mbsf::DistSessionState::Enum state);
    void continueStateTransitions(const std::shared_ptr<UserDataIngSession> &ing_sess);

    void setMbsDistributionSessionInfo(
                std::shared_ptr<reftools::mbsf::ObjDistributionOperatingMode> operating_mode,
                std::shared_ptr< reftools::mbsf::ObjAcquisitionMethod > obj_acquisition_method,
                const reftools::mbsf::ObjectDistrMethInfo::ObjAcqIdsType &acq_ids,
                const std::shared_ptr< reftools::mbsf::DistributionMethod > distribution_method,
                const std::string &ssm_source_address, const std::string &ssm_destination_address,
                const std::string &mbr,
                std::optional<std::shared_ptr< reftools::mbsf::DistSessionState > > dist_session_state = std::nullopt);

    const std::shared_ptr<reftools::mbsf::ObjectDistrMethInfo > populateObjectDistrMethInfo(std::shared_ptr<reftools::mbsf::ObjDistributionOperatingMode> operating_mode, std::shared_ptr< reftools::mbsf::ObjAcquisitionMethod > obj_acquisition_method, const reftools::mbsf::ObjectDistrMethInfo::ObjAcqIdsType &acq_ids);
    const std::shared_ptr<reftools::mbsf::MbsSessionId > populateMbsSessionId(const std::string &ssm_source_address, const std::string &ssm_destination_address);
    const std::shared_ptr<reftools::mbsf::Ssm> populateSsm(const std::string &ssm_source_address, const std::string &ssm_destination_address);


    std::shared_ptr<reftools::mbsf::MBSDistributionSessionInfo> m_mbsDistributionSessionInfo;
    std::map<std::string, DistributionSessionInfoSubscription> m_eventSubscriptions;
    SubscribedEvents m_eventTimestamps;
    std::shared_ptr<reftools::mbsf::StatusNotifyReqData> m_statusNotifyReqData;
    std::unique_ptr<DistributionSessionInfoSubscription> m_mbsDistributionSessionInfoSubscription;
    std::shared_ptr<AvailabilityInfo> m_availabilityInfo;
    std::recursive_mutex m_mutex;
    bool m_dataIngestSessionEstablished;
    bool m_dataIngestSessionTerminated;
    bool m_dataIngestSessionActivated;
    bool m_dataIngestSessionDeactivated;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_DISTRIBUTION_SESSION_HH_ */
