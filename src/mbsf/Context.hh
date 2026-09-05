#ifndef _MBSF_CONTEXT_HH_
#define _MBSF_CONTEXT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBSF Context
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

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

#include "ogs-sbi.h"
#include "ogs-app.h"

#include <SockAddr.hh>

#include "common.hh"
#include "UserDataIngSession.hh"
#include "openapi/model/MbsSessionId.h"

namespace reftools::mbsf {
    class MbsServiceArea;
    class ExternalMbsServiceArea;
}

namespace reftools::common::httpxpp {
    class HTTPServer;
    class HTTPRequestHandler;
}

MBSF_NAMESPACE_START

class Open5GSSBIServer;
class Open5GSSBIClient;
class Open5GSSockAddr;
class Open5GSYamlIter;
class UserService;
class UserServiceAnnChannel;
class UserDataIngStatSubsc;
class UniqueMbsSessionId;

class Context {
public:
    Context();
    Context(Context &&other) = delete;
    Context(const Context &other) = delete;
    Context &operator=(Context &&other) = delete;
    Context &operator=(const Context &other) = delete;
    virtual ~Context();

    bool parseConfig();

    std::vector <std::shared_ptr<Open5GSSockAddr> > MBSFUserServicesAddresses();
    std::vector <std::shared_ptr<Open5GSSockAddr> > MBSFUserDataIngestSessionAddresses();
    std::vector <std::shared_ptr<Open5GSSBIServer> > MBSFNotificationServers();

    void addUserService(const std::shared_ptr<UserService> &userService);
    void deleteUserService(const std::string &userServiceId);
    const std::shared_ptr<UserService> &findUserService(const std::string &id) const;

    void addUserDataIngSession(const std::shared_ptr<UserDataIngSession> &userIngSession);
    void deleteUserDataIngSession(const std::string &userIngSessionId);
    const std::shared_ptr<UserDataIngSession> &findUserDataIngSession(const std::string &id) const;

    void addMbsSessionId(const UniqueMbsSessionId &mbs_session_id);
    void addMbsSessionId(bool request_tmgi, const std::shared_ptr<reftools::mbsf::MbsSessionId> &mbs_session_id,
                         const std::shared_ptr<reftools::mbsf::MbsServiceArea> &mbs_service_area,
                         const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &ext_mbs_service_area);
    void deleteMbsSessionId(const UniqueMbsSessionId &mbs_session_id);
    void deleteMbsSessionId(bool request_tmgi, const std::shared_ptr<reftools::mbsf::MbsSessionId> &mbs_session_id,
                            const std::shared_ptr<reftools::mbsf::MbsServiceArea> &mbs_service_area,
                            const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &ext_mbs_service_area);
    bool haveMbsSessionId(const UniqueMbsSessionId &mbs_session_id) const;
    bool haveMbsSessionId(bool request_tmgi, const std::shared_ptr<reftools::mbsf::MbsSessionId> &mbs_session_id,
                          const std::shared_ptr<reftools::mbsf::MbsServiceArea> &mbs_service_area,
                          const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &ext_mbs_service_area) const;

    void addUserDataIngStatSubsc(const std::shared_ptr<UserDataIngStatSubsc> &userIngStatSubsc);
    void deleteUserDataIngStatSubsc(const std::string &userIngStatSubscId);

    int load();

    std::string assignNotificationServer(const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &id);
    void freeNotificationServer(const std::string &notif_url);
    const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &findDistSessIdFromUrl(const std::string &notif_url) const;
    std::shared_ptr<Open5GSSBIServer> newSbiServer(const ogs_sockaddr_t *address);

    bool userServiceAnnouncementConfigured();
    bool broadcastDistributionConfigured() const {
        return !broadcastDistribution.sourceAddress.empty() && !broadcastDistribution.destinationAddress.empty();
    }
    int32_t incAnnChannelCounter();
    int32_t decAnnChannelCounter();
    int32_t updateAnnChannelCounter(bool new_user_service_ann_channel, bool old_user_service_ann_channel);
    int32_t annChannelCount();
    void setUserServiceAnnouncementChannel();

    std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &userDataIngStatSubscs() { return m_userDataIngStatSubscs;};

    const std::string &userServiceAnnSsmSourceAddress() const { return userServiceAnnouncement.ssmSourceAddress;};
    const std::string &userServiceAnnSsmDestinationAddress() const { return userServiceAnnouncement.ssmDestinationAddress;};
    const std::string &userServiceAnnMbr() const { return userServiceAnnouncement.mbr;};
    unsigned int userServiceAnnSsmPort() const { return userServiceAnnouncement.ssmPort;};
    const std::string &userServiceAnnDocRoot() const { return userServiceAnnouncement.docRoot;};
    const std::string &broadcastDistributionSourceAddress() const { return broadcastDistribution.sourceAddress;};
    const std::string &broadcastDistributionDestinationAddress() const { return broadcastDistribution.destinationAddress;};
    std::optional<int32_t > repetitionInterval() const { return userServiceAnnouncement.announcementRepetitionTime;};
    std::optional<int32_t > keepUpdated() const { return userServiceAnnouncement.keepUpdatedInterval;};
    const std::shared_ptr<UserServiceAnnChannel> &userServiceAnnouncementChannel() const;
    const reftools::common::httpxpp::SockAddr &findUserServAnnServerAddrForRemote(const reftools::common::httpxpp::SockAddr &remote_addr) const;

    enum ServerType {
        OPEN5GS_SBI_SERVER,
        MBS_USER_SERVICES,
        MBS_USER_DATA_INGEST_SESSION,
        MBS_NOTIFICATION_LISTENER,
        SERVER_MAX_NUM
    };

    bool serverIsType(const Open5GSSBIServer &server, ServerType typ) const;

    std::map<std::string, std::shared_ptr<UserService> > UserServices;

    std::vector<std::shared_ptr<Open5GSSBIServer> > servers[SERVER_MAX_NUM];

    struct {
        unsigned int defaultMaxAge;
        unsigned int MBSUserServiceMaxAge;
        unsigned int MBSUserDataIngestSessionMaxAge;
    } cacheControl;

    struct {
        int activeDistributionSessionsSoftLimit;
        int activeUserServicesSoftLimit;
    } capacity;

    struct {
        int32_t backOffParametersOffsetTime;
        int32_t backOffParametersRandomTimePeriod;
        std::optional<std::string > objectRepairBaseLocator;
    } objectRepairParameters;

    struct {
        std::string mbr;
        std::optional<int32_t > announcementRepetitionTime;
        std::optional<int32_t > keepUpdatedInterval;
        std::string ssmSourceAddress;
        std::string ssmDestinationAddress;
        unsigned int ssmPort;
        std::string docRoot;
    } userServiceAnnouncement;

    // TS 26.502 V18.6.0 cl.4.5.6 (MBS Distribution Session parameters), Annex B.3.1: "The MBSF
    // nominates the MBS-4-MC multicast group destination IP address and UDP ports to be used
    // inside the Nmb9 unicast tunnel in the User plane traffic flow information." Used when a
    // Distribution Session's own MBS Session ID carries a TMGI but no AF-supplied SSM (a genuine
    // Broadcast session, per TS29571_CommonData.yaml's MbsSessionId anyOf[tmgi, ssm] and TS 29.580
    // cl.5.3.2.2.2) -- there is then no AF-nominated address to derive an Nmb9 label from, and per
    // the clause above this MBSF must nominate its own instead of leaving the label unset. Mirrors
    // userServiceAnnouncement's own ssmSourceAddress/ssmDestinationAddress config pattern, a
    // different, single, MBSF-internal distribution session with the same "MBSF nominates its own"
    // shape. Port is not configured here: mirrors userServiceAnnouncement's approach of one static
    // config value where only one such session exists, but per-Broadcast-session ContextData
    // already draws its own port at random (see ContextData::ssm_port) for exactly the
    // multiple-concurrent-sessions uniqueness this fixed address alone cannot provide.
    struct {
        std::string sourceAddress;
        std::string destinationAddress;
    } broadcastDistribution;

    std::int64_t actPeriodEstablishedStateDuration = 60;
    int32_t userServicesWithViaMbsDistSession = 0;

    std::optional<std::string> allowedMulticastRange;

    // TS 29.500 V18.10.0 cl.5.2.7.2/table 5.2.7.1-1: 413 (Payload Too Large) is mandatory for
    // PATCH and POST. No clause, and no MBSF documented default, names a byte limit
    // -- unset means no limit is enforced, as before this option existed. Only partially closes
    // the requirement even when set: the shared open5gs SBI server silently truncates bodies
    // past its own OGS_MAX_SDU_LEN before this check (or any NF's) ever runs -- see
    // rt-mbs-transport-function.md's own M8 entry for the full account of that residual gap.
    std::optional<size_t> maxRequestBodySize;

    ogs_sockaddr_t *notificationBindAddress;

private:
    void parseCacheControl(Open5GSYamlIter &iter);
    void parseConfiguration(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseUserServAnnSvrConfiguration(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseObjectRepairParameters(Open5GSYamlIter &iter);
    int parseNotificationConfig(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseUserServiceAnnouncement(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseBroadcastDistribution(Open5GSYamlIter &iter);
    void configureMBSAF(const std::string &pc_key, Open5GSYamlIter &iter);
    std::shared_ptr<Open5GSSBIServer> getServerForAddr(const ogs_sockaddr_t *addr, int add_to_server_type);
    const std::shared_ptr<Open5GSSBIServer> &findServerForAddr(const ogs_sockaddr_t *addr) const;
    const std::shared_ptr<Open5GSSBIServer> &findServerForAddr(const ogs_socknode_t *node) const;
    void startUserServAnnServers();
    void createUserServAnnRequestHandler();

    std::shared_ptr<std::recursive_mutex> m_userDataIngSessMutex;
    std::map<std::string, std::weak_ptr<UserService> > m_userDataIngSessIndex;

    std::shared_ptr<std::recursive_mutex> m_mbsSessionIdsMutex;
    std::set<UniqueMbsSessionId> m_mbsSessionIds;

    std::shared_ptr<std::recursive_mutex> m_userDataIngStatSubscMutex;
    std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > m_userDataIngStatSubscs;

    std::shared_ptr<std::recursive_mutex> m_notifServerMapMutex;
    std::map<std::string, std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> > m_notifServerMap;

    std::shared_ptr<std::recursive_mutex> m_userServiceAnnChannelMutex;
    std::shared_ptr<UserServiceAnnChannel> m_userServiceAnnChannel;

    std::shared_ptr<reftools::common::httpxpp::HTTPRequestHandler> m_userServAnnRequestHandler;
    std::unordered_set<reftools::common::httpxpp::SockAddr> m_userServAnnAddresses;
    std::list<std::shared_ptr<reftools::common::httpxpp::HTTPServer> > m_userServAnnServers;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_CONTEXT_HH_ */
