#ifndef _MBS_TF_OPEN5GS_NETWORK_FUNCTION_HH_
#define _MBS_TF_OPEN5GS_NETWORK_FUNCTION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <list>

#include "common.hh"
#include "Open5GSYamlDocument.hh"

MBSF_NAMESPACE_START

class Open5GSEvent;
class Open5GSTimer;
class Open5GSSockAddr;
class TimerFunc;

class Open5GSNetworkFunction {
public:
    class Service {
    public:
	Service() = delete;    
        Service(const char *serviceName, const char *supportedFeatures, const char *apiVersion, const std::vector<std::shared_ptr<Open5GSSockAddr> > &addr);
	Service(const char *serviceName, const char *supportedFeatures, const char *apiVersion, const std::vector<std::shared_ptr<Open5GSSockAddr> > &addr, const std::optional<int> capacity = std::nullopt);
	Service(Service &other) = delete;
	Service(Service &&other) = delete;
	Service &operator=(Service &&other) = delete;
        Service &operator=(const Service &other) = delete;

	virtual ~Service() {};
	const char *serviceName() const { return m_serviceName; };
	const char *supportedFeatures() const { return m_supportedFeatures;};
	const char *apiVersion() const { return m_apiVersion; };
	const std::optional<int> capacity() const { return m_capacity; };
	const std::vector<std::shared_ptr<Open5GSSockAddr> > &sockAddrs() const {return m_addrs;};

    private:
        const char *m_serviceName;
        const char *m_supportedFeatures;
        const char *m_apiVersion;
        const std::vector<std::shared_ptr<Open5GSSockAddr> > m_addrs;
	const std::optional<int> m_capacity;
    };

    Open5GSNetworkFunction();
    Open5GSNetworkFunction(Open5GSNetworkFunction &&other) = delete;
    Open5GSNetworkFunction(const Open5GSNetworkFunction &other) = delete;
    Open5GSNetworkFunction &operator=(Open5GSNetworkFunction &&other) = delete;
    Open5GSNetworkFunction &operator=(const Open5GSNetworkFunction &other) = delete;
    virtual ~Open5GSNetworkFunction();

    bool noIPv4() const { return ogs_global_conf()->parameter.no_ipv4; };
    bool noIPv6() const { return ogs_global_conf()->parameter.no_ipv6; };

    std::shared_ptr<Open5GSTimer> addTimer(TimerFunc &timer_func);
    void removeTimer(const std::shared_ptr<Open5GSTimer> &timer);

    Open5GSYamlDocument configFileDocument() const;

    int pushEvent(const std::shared_ptr<Open5GSEvent> &event);

    bool configureLoggingDomain();
    bool sbiParseConfig(const char *app_section, const char *nrf_section = "nrf", const char *scp_section = "scp");
    bool sbiOpen();
    void sbiClose();

    bool startEventHandler();
    void stopEventHandler();

    bool addNFService(const char *serviceName, const char *supportedFeatures, const char *apiVersion, const std::vector<std::shared_ptr<Open5GSSockAddr> > &addr, std::optional<int> capacity = std::nullopt);
    int setNFServices(const char *serviceName, const char *supportedFeatures, const char *apiVersion, const std::vector<std::shared_ptr<Open5GSSockAddr> > &addrs, std::optional<int> capacity = std::nullopt);

    void initialise() {ogs_sbi_context_init(nfType());};

    virtual OpenAPI_nf_type_e nfType() const { return OpenAPI_nf_type_AF; };
    const std::string &serverName();
    const std::string &serverName() const { return m_serverName; };


private:
    int setServerName();
    static void eventThread(void *data);
    static void addAddressesToNFService(ogs_sbi_nf_service_t *nf_service, const std::vector<std::shared_ptr<Open5GSSockAddr> > &addrs);

    static void stateInitial(ogs_fsm_t *s, ogs_event_t *e);
    static void stateFunctional(ogs_fsm_t *s, ogs_event_t *e);
    static void stateFinal(ogs_fsm_t *s, ogs_event_t *e);

    ogs_thread_t *m_eventThread;
    std::list<Service> m_nfServices;
    std::string m_serverName;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_NETWORK_FUNCTION_HH_ */
