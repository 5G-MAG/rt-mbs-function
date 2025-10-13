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
#include <vector>
#include <optional>

#include "ogs-sbi.h"
#include "ogs-app.h"

#include "common.hh"
#include "MBSMFMBSSession.hh"

MBSF_NAMESPACE_START

class MBSSession;
class Open5GSSBIServer;
class Open5GSSBIClient;
class Open5GSSockAddr;
class Open5GSYamlIter;
class UserService;
class UserDataIngSession;

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

    void addUserService(const std::shared_ptr<UserService> &userService);
    void deleteUserService(const std::string &userServiceId);
    void addUserDataIngSession(const std::shared_ptr<UserDataIngSession> &userIngSession);
    void deleteUserDataIngSession(const std::string &userIngSessionId);
    void addMbSmfMbsSession(const std::shared_ptr<MBSMFMBSSession> &session);
    void deleteMbSmfMbsSession(const mb_smf_sc_ssm_addr_t *ssm);
    int load();

    enum ServerType {
        MBS_USER_SERVICES,
        MBS_USER_DATA_INGEST_SESSION,
        SERVER_MAX_NUM
    };

    std::map<std::string, std::shared_ptr<UserService> > UserServices;
    std::map<std::string, std::shared_ptr<UserDataIngSession> > UserDataIngSessions;
    std::map<const mb_smf_sc_ssm_addr_t *, std::shared_ptr<MBSMFMBSSession> > MBSMFMBSSessions;

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


private:
    void parseCacheControl(Open5GSYamlIter &iter);
    void parseConfiguration(std::string &pc_key, Open5GSYamlIter &iter);
    int checkForAddr(ogs_socknode_t *node);

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_CONTEXT_HH_ */
