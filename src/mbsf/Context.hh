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

#include "ogs-sbi.h"
#include "ogs-app.h"

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSBIServer;
class Open5GSSBIClient;
class Open5GSSockAddr;
class Open5GSYamlIter;

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

    enum ServerType {
	MBS_USER_SERVICES,
	MBS_USER_DATA_INGEST_SESSION,
        SERVER_MAX_NUM
    };
    
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

private:
    void parseCacheControl(Open5GSYamlIter &iter);
    void parseConfiguration(std::string &pc_key, Open5GSYamlIter &iter);
    int checkForAddr(ogs_socknode_t *node);

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_CONTEXT_HH_ */
