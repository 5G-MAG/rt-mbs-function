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
#include <vector>

#include "ogs-sbi.h"
#include "ogs-app.h"

#include "common.hh"
#include "openapi/model/MbsSessionId.h"

namespace reftools::mbsf {
    class MbsServiceArea;
    class ExternalMbsServiceArea;
}

MBSF_NAMESPACE_START

class Open5GSSBIServer;
class Open5GSSBIClient;
class Open5GSSockAddr;
class Open5GSYamlIter;
class UserService;
class UserDataIngSession;
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

    void assignNotificationServer();
    const ogs_sockaddr_t *contextGetNotificationAddress();
    ogs_sbi_server_t *newSbiServer(const ogs_sockaddr_t *address);

     std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > &userDataIngStatSubscs() { return m_userDataIngStatSubscs;};


    enum ServerType {
        OPEN5GS_SBI_SERVER,
        MBS_USER_SERVICES,
        MBS_USER_DATA_INGEST_SESSION,
	MBS_NOTIFICATION_LISTENER,
        SERVER_MAX_NUM
    };

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

    std::int64_t actPeriodEstablishedStateDuration = 60;

    std::optional<std::string> allowedMulticastRange;

    ogs_sockaddr_t *notification_bind_address;

private:
    void parseCacheControl(Open5GSYamlIter &iter);
    void parseConfiguration(std::string &pc_key, Open5GSYamlIter &iter);
    int parseNotificationConfig(std::string &pc_key, Open5GSYamlIter &iter);
    const std::shared_ptr<Open5GSSBIServer> &findServerForAddr(ogs_socknode_t *node);

    std::shared_ptr<std::recursive_mutex> m_userDataIngSessMutex;
    std::map<std::string, std::weak_ptr<UserService> > m_userDataIngSessIndex;

    std::shared_ptr<std::recursive_mutex> m_mbsSessionIdsMutex;
    std::set<UniqueMbsSessionId> m_mbsSessionIds;

    std::shared_ptr<std::recursive_mutex> m_userDataIngStatSubscMutex;
    std::map<std::string, std::shared_ptr<UserDataIngStatSubsc> > m_userDataIngStatSubscs;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_CONTEXT_HH_ */
