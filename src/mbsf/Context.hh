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
    void setUserServiceAnnouncementChannel();

    std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &userDataIngStatSubscs() { return m_userDataIngStatSubscs;};

    const std::string &userServiceAnnSsmSourceAddress() const { return userServiceAnnouncement.ssmSourceAddress;};
    const std::string &userServiceAnnSsmDestinationAddress() const { return userServiceAnnouncement.ssmDestinationAddress;};
    const std::string &userServiceAnnMbr() const { return userServiceAnnouncement.mbr;};
    unsigned int userServiceAnnSsmPort() const { return userServiceAnnouncement.ssmPort;};
    const std::string &userServiceAnnDocRoot() const { return userServiceAnnouncement.docRoot;};
    const std::shared_ptr<UserServiceAnnChannel> &userServiceAnnouncementChannel() const { return m_userServiceAnnChannel; };
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
        std::optional<unsigned int > announcementRepetitionTime;
        std::string ssmSourceAddress;
        std::string ssmDestinationAddress;
        unsigned int ssmPort;
        std::string docRoot;
    } userServiceAnnouncement;

    std::int64_t actPeriodEstablishedStateDuration = 60;

    std::optional<std::string> allowedMulticastRange;

    ogs_sockaddr_t *notificationBindAddress;

private:
    void parseCacheControl(Open5GSYamlIter &iter);
    void parseConfiguration(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseUserServAnnSvrConfiguration(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseObjectRepairParameters(Open5GSYamlIter &iter);
    int parseNotificationConfig(const std::string &pc_key, Open5GSYamlIter &iter);
    void parseUserServiceAnnouncement(const std::string &pc_key, Open5GSYamlIter &iter);
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
