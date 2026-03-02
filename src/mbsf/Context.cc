/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: App context class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
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
#include <string>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "ogs-core.h"
#include "ogs-sbi.h"
#include "ogs-app.h"

#include "common.hh"
#include "App.hh"
#include "Open5GSNetworkFunction.hh"
#include "Open5GSSBIHeader.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSockAddr.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSYamlIter.hh"
#include "UserDataIngStatSubsc.hh"
#include "UserService.hh"
#include "UserDataIngSession.hh"
#include "UniqueMBSSessionId.hh"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/MbsSessionId.h"

#include "Context.hh"

using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsSessionId;

extern ogs_sbi_server_actions_t ogs_sbi_server_actions;

MBSF_NAMESPACE_START

Context::Context()
    :servers()
    ,cacheControl({60, 60, 60})
    ,capacity({100,100})
    ,allowedMulticastRange()
    ,notificationBindAddress(nullptr)
    ,m_userDataIngSessMutex(new std::recursive_mutex)
    ,m_userDataIngSessIndex()
    ,m_mbsSessionIdsMutex(new std::recursive_mutex)
    ,m_mbsSessionIds()
    ,m_userDataIngStatSubscMutex(new std::recursive_mutex)
    ,m_userDataIngStatSubscs()
    ,m_notifServerMapMutex(new std::recursive_mutex)
    ,m_notifServerMap()
{
}

Context::~Context()
{
    for (auto &svrs : servers) {
        for (auto &svr: svrs) {
            svr.reset();
        }
    }
    {
        std::lock_guard<decltype(m_userDataIngSessMutex)::element_type> lock(*m_userDataIngSessMutex);
        m_userDataIngSessIndex.clear();
    }
    UserDataIngSession::clearRegistries();
}

bool Context::parseConfig()
{
    ogs_sbi_server_t *server;
    ogs_list_t *sbi_servers = &ogs_sbi_self()->server_list;
    ogs_list_for_each(sbi_servers, server) {
        servers[OPEN5GS_SBI_SERVER].emplace_back(new Open5GSSBIServer(server));
    }
    Open5GSYamlDocument doc(App::self().configDocument());
    Open5GSYamlIter root_iter(doc);
    while (root_iter.next()) {
        std::string root_key(root_iter.key());
        if (root_key == "mbsf") {
            Open5GSYamlIter mbsf_iter(root_iter);
            while (mbsf_iter.next()) {
                std::string mbsf_key(mbsf_iter.key());
                if (mbsf_key == "sbi" || mbsf_key == "service_name" || mbsf_key == "discovery") {
                    // Handled by SBI config parser
                } else if (mbsf_key == "serverResponseCacheControl") {
                    Open5GSYamlIter cc_array(mbsf_iter);
                    if (cc_array.type() == YAML_MAPPING_NODE) {
                        parseCacheControl(cc_array);
                    } else if (cc_array.type() == YAML_SEQUENCE_NODE) {
                        if (!cc_array.next()) break;
                        Open5GSYamlIter cc_iter(cc_array);
                        parseCacheControl(cc_iter);
                    } else if (cc_array.type() == YAML_SCALAR_NODE) {
                        break;
                    } else {
                        throw std::out_of_range("Bad configuration node at mbsf.serverResponseCacheControl");
                     }

                } else if (mbsf_key == "activeDistributionSessionsSoftLimit") {
                      std::string active_distribution_sessions_limit(mbsf_iter.value());
                      capacity.activeDistributionSessionsSoftLimit = std::stoi(active_distribution_sessions_limit);

                } else if (mbsf_key == "activeUserServicesSoftLimit") {
                    std::string active_user_services_limit(mbsf_iter.value());
                    capacity.activeUserServicesSoftLimit = std::stoi(active_user_services_limit);
                } else if (mbsf_key == "actPeriodGoToEstablishedState") {
                     std::string act_period_established_state_dur(mbsf_iter.value());
                     actPeriodEstablishedStateDuration = std::stoll( act_period_established_state_dur);
                } else if (mbsf_key == "allowedMulticastRange" ) {
                    allowedMulticastRange = std::string(mbsf_iter.value());
                } else if (mbsf_key == "mbsUserServices" || mbsf_key == "mbsUserDataIngestSession") {
                    Open5GSYamlIter mbsUserServices_array(mbsf_iter);
                    do {
                        if (mbsUserServices_array.type() == YAML_MAPPING_NODE) {
                            parseConfiguration(mbsf_key, mbsUserServices_array);
                        } else if (mbsUserServices_array.type() == YAML_SEQUENCE_NODE) {
                            if (!mbsUserServices_array.next()) break;
                            Open5GSYamlIter mbsUserServices_iter(mbsUserServices_array);
                            parseConfiguration(mbsf_key, mbsUserServices_iter);
                        } else if (mbsUserServices_array.type() == YAML_SCALAR_NODE) {
                            break;
                        } else {
                            throw std::out_of_range(std::format("Bad configuration node at mbsf.{}", mbsf_key));
                        }

                    } while (mbsUserServices_array.type() == YAML_SEQUENCE_NODE);
                } else if (mbsf_key == "notificationListener") {
                    Open5GSYamlIter notificationListener_iter(mbsf_iter);
                    do {
                        if (notificationListener_iter.type() == YAML_MAPPING_NODE) {
                            if (parseNotificationConfig("mbsf.notificationListener", notificationListener_iter) != OGS_OK) {
                                ogs_warn("Problem in mbsf.notificationListener section of configuration");
                            }
                        } else if (notificationListener_iter.type() == YAML_SEQUENCE_NODE) {
                            if (!notificationListener_iter.next()) break;
                            Open5GSYamlIter notificationListener_array_iter(notificationListener_iter);
                            if (parseNotificationConfig("mbsf.notificationListener", notificationListener_array_iter) != OGS_OK) {
                                ogs_warn("Problem in mbsf.notificationListener section of configuration");
                            }
                        } else if (notificationListener_iter.type() == YAML_SCALAR_NODE) {
                            break;
                        } else {
                            throw std::out_of_range(std::format("Bad configuration node at mbsf.{}", mbsf_key));
                        }
                    } while (notificationListener_iter.type() == YAML_SEQUENCE_NODE);
                } else {
                    ogs_warn("Unknown key `mbsf.%s` in configuration", mbsf_key.c_str());
                }
            }
        }
    }

    return true;
}

void Context::addUserService(const std::shared_ptr<UserService> &service)
{
    std::shared_ptr<UserService> map_service(service);
    UserServices.insert(std::make_pair<std::string, std::shared_ptr<UserService> >(std::string(map_service->userServiceId()), std::move(map_service)));
}


void Context::deleteUserService(const std::string &id)
{
    auto it = UserServices.find(id);
    if (it != UserServices.end()) {
        UserServices.erase(it);
    } else {
        throw std::out_of_range("MBSF: User Service not found");
    }
}

const std::shared_ptr<UserService> &Context::findUserService(const std::string &id) const
{
    auto it = UserServices.find(id);
    if (it != UserServices.end()) {
        return it->second;
    }
    static const std::shared_ptr<UserService> null_us;
    return null_us;
}

void Context::addUserDataIngSession(const std::shared_ptr<UserDataIngSession> &session)
{
    if (!session) return;
    auto &mbs_user_data_ing_session = session->getMBSUserIngSession();
    auto &mbs_user_service_id = mbs_user_data_ing_session->getMbsUserServId();
    auto it = UserServices.find(mbs_user_service_id);
    if (it == UserServices.end()) {
        throw std::out_of_range(std::format("User Service {} not found when adding User Data Ingest Session", mbs_user_service_id));
    }
    auto &mbs_user_service = it->second;
    mbs_user_service->addUserDataIngSession(session);
    std::lock_guard<std::recursive_mutex> lock(*m_userDataIngSessMutex);
    std::remove_reference<decltype(mbs_user_service)>::type::weak_type weak_service(mbs_user_service);
    m_userDataIngSessIndex.insert(std::make_pair<decltype(m_userDataIngSessIndex)::key_type, decltype(m_userDataIngSessIndex)::mapped_type>(std::string(session->userDataIngSessionId()), std::move(weak_service)));
}


void Context::deleteUserDataIngSession(const std::string &id)
{
    std::lock_guard<std::recursive_mutex> lock(*m_userDataIngSessMutex);
    auto it = m_userDataIngSessIndex.find(id);
    if (it != m_userDataIngSessIndex.end()) {
        auto mbs_user_service = it->second.lock();
        if (mbs_user_service) mbs_user_service->deleteUserDataIngSession(id);
        m_userDataIngSessIndex.erase(it);
    } else {
        throw std::out_of_range("MBSF: User Ingest Session to be deleted is not found");
    }
}

const std::shared_ptr<UserDataIngSession> &Context::findUserDataIngSession(const std::string &id) const
{
    std::lock_guard<std::recursive_mutex> lock(*m_userDataIngSessMutex);
    auto it = m_userDataIngSessIndex.find(id);
    if (it != m_userDataIngSessIndex.end()) {
        auto mbs_user_service = it->second.lock();
        if (mbs_user_service) {
            return mbs_user_service->findUserDataIngSession(id);
        }
    }
    static const std::shared_ptr<UserDataIngSession> null_udis(nullptr);
    return null_udis;
}

void Context::addMbsSessionId(const UniqueMbsSessionId &mbs_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(*m_mbsSessionIdsMutex);
    auto [it, inserted] = m_mbsSessionIds.insert(mbs_session_id);
    if (!inserted) {
        ogs_warn("Attempt to insert duplicate %s into context", mbs_session_id.repr().c_str());
    }
}

void Context::addMbsSessionId(bool request_tmgi, const std::shared_ptr<MbsSessionId> &mbs_session_id,
                              const std::shared_ptr<MbsServiceArea> &mbs_svc_area,
                              const std::shared_ptr<ExternalMbsServiceArea> &ext_mbs_svc_area)
{
    addMbsSessionId(UniqueMbsSessionId(request_tmgi, mbs_session_id, mbs_svc_area, ext_mbs_svc_area));
}

void Context::deleteMbsSessionId(const UniqueMbsSessionId &mbs_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(*m_mbsSessionIdsMutex);
    if (!m_mbsSessionIds.erase(mbs_session_id)) {
        ogs_warn("Attempt to delete non-existant %s from context", mbs_session_id.repr().c_str());
    }
}

void Context::deleteMbsSessionId(bool request_tmgi, const std::shared_ptr<MbsSessionId> &mbs_session_id,
                                 const std::shared_ptr<MbsServiceArea> &mbs_svc_area,
                                 const std::shared_ptr<ExternalMbsServiceArea> &ext_mbs_svc_area)
{
    deleteMbsSessionId(UniqueMbsSessionId(request_tmgi, mbs_session_id, mbs_svc_area, ext_mbs_svc_area));
}

bool Context::haveMbsSessionId(const UniqueMbsSessionId &mbs_session_id) const
{
    std::lock_guard<std::recursive_mutex> lock(*m_mbsSessionIdsMutex);
    return m_mbsSessionIds.contains(mbs_session_id);
}

bool Context::haveMbsSessionId(bool request_tmgi, const std::shared_ptr<MbsSessionId> &mbs_session_id,
                               const std::shared_ptr<MbsServiceArea> &mbs_svc_area,
                               const std::shared_ptr<ExternalMbsServiceArea> &ext_mbs_svc_area) const
{
    return haveMbsSessionId(UniqueMbsSessionId(request_tmgi, mbs_session_id, mbs_svc_area, ext_mbs_svc_area));
}

void Context::parseCacheControl(Open5GSYamlIter &iter) {
    while (iter.next()) {
        std::string cc_key(iter.key());
        std::string cc_val(iter.value());
        try {
            if (cc_key == "defaultMaxAge") {
                cacheControl.defaultMaxAge = std::stol(cc_val);
            } else if (cc_key == "mbsUserServiceMaxAge") {
                cacheControl.MBSUserServiceMaxAge = std::stol(cc_val);
            } else if (cc_key == "mbsUserDataIngestSessionMaxAge") {
                cacheControl.MBSUserDataIngestSessionMaxAge = std::stol(cc_val);
            }
        } catch (std::out_of_range &ex) {
            ogs_error("Cache control value for %s of \"%s\" is too big for integer storage.", cc_key.c_str(), cc_val.c_str());
        } catch (std::invalid_argument &ex) {
            ogs_error("Cache control value for %s of \"%s\" is not understood as an integer.", cc_key.c_str(), cc_val.c_str());
        }
    }
}

void Context::addUserDataIngStatSubsc(const std::shared_ptr<UserDataIngStatSubsc> &subsc)
{
    std::lock_guard<std::recursive_mutex> lock(*m_userDataIngStatSubscMutex);
    std::shared_ptr<UserDataIngStatSubsc> map_subsc(subsc);
    m_userDataIngStatSubscs.insert(std::make_pair<std::string, std::shared_ptr<UserDataIngStatSubsc> >(std::string(map_subsc->subscriptionId()), std::move(map_subsc)));
}

void Context::deleteUserDataIngStatSubsc(const std::string &id)
{
    std::lock_guard<std::recursive_mutex> lock(*m_userDataIngStatSubscMutex);
    auto it = m_userDataIngStatSubscs.find(id);
    if (it != m_userDataIngStatSubscs.end()) {
        m_userDataIngStatSubscs.erase(it);
    } else {
        throw std::out_of_range("MBSF: User Ingest Stat Subsc to be deleted is not found");
    }
}

void Context::parseConfiguration(const std::string &pc_key, Open5GSYamlIter &iter)   {
     ogs_list_t list, list6;
     ogs_socknode_t *node = NULL, *node6 = NULL;
     int rv;
     int i, family = AF_UNSPEC;
     int num = 0;
     const char *hostname[OGS_MAX_NUM_OF_HOSTNAME];
     int num_of_advertise = 0;
     const char *advertise[OGS_MAX_NUM_OF_HOSTNAME];
     uint16_t port = 0;
     const char *dev = NULL;
     ogs_sockaddr_t *addr = NULL;
     ogs_sockopt_t option;
     bool is_option = false;

     while (iter.next()) {
         std::string sbi_key(iter.key());
         if(sbi_key == "family") {
             const char *v = iter.value();
             if (v) family = atoi(v);
             if (family != AF_UNSPEC && family != AF_INET && family != AF_INET6) {
                 ogs_warn("Ignore family(%d) : ""AF_UNSPEC(%d), " "AF_INET(%d), AF_INET6(%d) ", family, AF_UNSPEC, AF_INET, AF_INET6);
                 family = AF_UNSPEC;
             }
         } else if ((sbi_key == "addr") || (sbi_key == "name")) {
             Open5GSYamlIter hostname_iter(iter);
             ogs_assert(hostname_iter.type() != YAML_MAPPING_NODE);
             do {
                    if (hostname_iter.type() == YAML_SEQUENCE_NODE) {
                        if (!hostname_iter.next()) break;
                    }
                    ogs_assert(num < OGS_MAX_NUM_OF_HOSTNAME);
                    hostname[num++] = hostname_iter.value();
                } while (hostname_iter.type() == YAML_SEQUENCE_NODE);
         } else if (sbi_key == "advertise") {
             Open5GSYamlIter advertise_iter(iter);
             ogs_assert(advertise_iter.type() != YAML_MAPPING_NODE);
             do {
                    if (advertise_iter.type() == YAML_SEQUENCE_NODE) {
                        if (!advertise_iter.next()) break;
                    }
                   ogs_assert(num_of_advertise < OGS_MAX_NUM_OF_HOSTNAME);
                   advertise[num_of_advertise++] = advertise_iter.value();
                } while (advertise_iter.type() == YAML_SEQUENCE_NODE);
        } else if (sbi_key == "port") {
             const char *v = iter.value();
             if (v) port = atoi(v);
        } else if (sbi_key == "dev") {
             dev = iter.value();
        } else if (sbi_key == "option") {
             is_option = true;
        } else if (sbi_key == "tls") {
            Open5GSYamlIter tls_iter(iter);
            while (tls_iter.next()) {
              std::string tls_key(tls_iter.key());
              if (tls_key == "key") {
                   //key = tls_iter.value();
               } else if (tls_key == "pem") {
                   //pem = tls_iter.value();
               } else
                   ogs_warn("unknown key `%s`", tls_key.c_str());
            }
        } else
            ogs_warn("unknown key `%s`", sbi_key.c_str());

    }
    if (port == 0){
        ogs_warn("Specify the [%s] port, otherwise a random port will be used", pc_key.c_str());
    }

    addr = NULL;
    for (i = 0; i < num; i++) {
        rv = ogs_addaddrinfo(&addr, family, hostname[i], port, 0);
        ogs_assert(rv == OGS_OK);
    }

    ogs_list_init(&list);
    ogs_list_init(&list6);

    if (addr) {
        if (ogs_global_conf()->parameter.no_ipv4 == 0)
            ogs_socknode_add(&list, AF_INET, addr, NULL);
        if (ogs_global_conf()->parameter.no_ipv6 == 0)
            ogs_socknode_add(&list6, AF_INET6, addr, NULL);
        ogs_freeaddrinfo(addr);
    }

    if (dev) {
        rv = ogs_socknode_probe(ogs_global_conf()->parameter.no_ipv4 ? NULL : &list,
                                ogs_global_conf()->parameter.no_ipv6 ? NULL : &list6,
                                dev, port, NULL);
        ogs_assert(rv == OGS_OK);
    }

    addr = NULL;
    for (i = 0; i < num_of_advertise; i++) {
        rv = ogs_addaddrinfo(&addr, family, advertise[i], port, 0);
        ogs_assert(rv == OGS_OK);
    }
    ogs_list_for_each(&list, node) {
        if (node) {
            std::shared_ptr<Open5GSSBIServer> new_server = findServerForAddr(node);

            if (!new_server) {
                new_server.reset(new Open5GSSBIServer(node, is_option ? &option : nullptr));
                new_server->ogsSBIServerAdvertise(addr);
            }

            if (pc_key == "mbsUserServices") {
                servers[MBS_USER_SERVICES].push_back(new_server);
            } else if (pc_key == "mbsUserDataIngestSession") {
                servers[MBS_USER_DATA_INGEST_SESSION].push_back(new_server);
	    }
        }
    }
    ogs_list_for_each(&list6, node6) {
        if (node6) {
            std::shared_ptr<Open5GSSBIServer> new_server = findServerForAddr(node);

            if(!new_server) {
                new_server.reset(new Open5GSSBIServer(node, is_option ? &option : nullptr));
                new_server->ogsSBIServerAdvertise(addr);
            }

            if (pc_key == "mbsUserServices") {
                servers[MBS_USER_SERVICES].push_back(new_server);
            } else if (pc_key == "mbsUserDataIngestSession") {
                servers[MBS_USER_DATA_INGEST_SESSION].push_back(new_server);
            }
        }
    }
    if (addr) ogs_freeaddrinfo(addr);
    ogs_socknode_remove_all(&list);
    ogs_socknode_remove_all(&list6);
}

std::vector <std::shared_ptr<Open5GSSockAddr> > Context::MBSFUserServicesAddresses()
{
    std::vector<std::shared_ptr<Open5GSSockAddr> > sockAddrs;
    std::vector<std::shared_ptr<Open5GSSBIServer>> srvs = servers[MBS_USER_SERVICES];
    if(!srvs.empty()) {
        for (const auto &srv: srvs) {
            sockAddrs.emplace_back(new Open5GSSockAddr(srv->ogsSBIServer()->node.addr));
        }
    } else {
        ogs_warn("No MBS User Services API servers configured");
    }
    return sockAddrs;
}

std::vector <std::shared_ptr<Open5GSSockAddr> > Context::MBSFUserDataIngestSessionAddresses()
{
    std::vector<std::shared_ptr<Open5GSSockAddr> > sockAddrs;
    std::vector<std::shared_ptr<Open5GSSBIServer>> srvs = servers[MBS_USER_DATA_INGEST_SESSION];
    if(!srvs.empty()) {
        for (const auto &srv: srvs) {
            sockAddrs.emplace_back(new Open5GSSockAddr(srv->ogsSBIServer()->node.addr));
        }
    } else {
        ogs_warn("No MBS User Data Ingest Session API servers configured");
    }
    return sockAddrs;
}


int Context::parseNotificationConfig(const std::string &pc_key, Open5GSYamlIter &iter) {
    const char *addr = nullptr;
    uint16_t port = 0;

    // Iterate through the keys inside the current block (e.g., address, port)
    while (iter.next()) {
        std::string key(iter.key());

        if (key == "addr") {
            addr = iter.value();
        } else if (key == "port") {
            const char *v = iter.value();
            if (v) {
                char *end_ptr = nullptr;
                unsigned long num = strtoul(v, &end_ptr, 0);

                if (!end_ptr || *end_ptr || num >= 65536) {
                    ogs_error("Notification port (%s) must be 0-65535", v);
		    return OGS_ERROR;
                } else {
                    port = (uint16_t)num;
                }
            }
        } else {
            ogs_warn("Unknown key `%s.%s`", pc_key.c_str(), key.c_str());
        }
    }

    // Process the collected configuration
    if (addr) {
        /* Resolve hostname/IP and fill notification_bind_address */
        if (ogs_getaddrinfo(&notificationBindAddress, AF_UNSPEC, addr, port,
                            AI_V4MAPPED | AI_ADDRCONFIG | AI_PASSIVE) != OGS_OK) {
            ogs_error("[%s.address] Could not get address info for %s", pc_key.c_str(), addr);
            return OGS_ERROR;
        }
    } else {
        /* Default to IPv4 Any (0.0.0.0) if only port is provided */
        if (notificationBindAddress) ogs_freeaddrinfo(notificationBindAddress);
        notificationBindAddress = (ogs_sockaddr_t*)ogs_calloc(1, sizeof(ogs_sockaddr_t));
        ogs_assert(notificationBindAddress);
        notificationBindAddress->ogs_sa_family = AF_INET;
        notificationBindAddress->ogs_sin_port = htons(port);
    }
    return OGS_OK;
}

std::string Context::assignNotificationServer(const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &dist_session_id)
{
    const ogs_sockaddr_t *notif_address = notificationBindAddress;

    if (!notif_address) {
        /* if not configured, use ephemeral port on IPv4 any address */
        static const ogs_sockaddr_t any_ephemeral_v4 = { .sa = { .sa_family = AF_INET } };
        notif_address = &any_ephemeral_v4;
    }

    Open5GSSBIHeader header;
    header.serviceName("notify");
    header.apiVersion("v1");

    std::shared_ptr<Open5GSSBIServer> notification_server(getServerForAddr(notif_address, MBS_NOTIFICATION_LISTENER));
    header.resourceComponent(0, dist_session_id->first.c_str());
    header.resourceComponent(1, dist_session_id->second.c_str());

    std::string notif_url = notification_server->makeUrl(header);
    if (!notif_url.empty()) {
        std::lock_guard<decltype(m_notifServerMapMutex)::element_type> lock(*m_notifServerMapMutex);
        m_notifServerMap.insert(decltype(m_notifServerMap)::value_type(notif_url, dist_session_id));
    }
    return notif_url;
}

void Context::freeNotificationServer(const std::string &notif_url)
{
    auto it = m_notifServerMap.find(notif_url);
    if (it != m_notifServerMap.end()) m_notifServerMap.erase(it);
}

std::shared_ptr<Open5GSSBIServer> Context::newSbiServer(const ogs_sockaddr_t *address)
{
    ogs_sbi_server_t *svr = ogs_sbi_server_add(NULL, OpenAPI_uri_scheme_http, (ogs_sockaddr_t*)address, NULL);
    if (svr) {
        ogs_sbi_server_actions.start(svr, ogs_sbi_server_handler);
        if ((address->ogs_sa_family == AF_INET && address->sin.sin_port == 0) ||
            (address->ogs_sa_family == AF_INET6 && address->sin6.sin6_port == 0)) {
            /* Retrieve bound address to get port number of ephemeral port */
            char buf[OGS_ADDRSTRLEN];
            socklen_t len = ogs_sockaddr_len(&svr->node.sock->local_addr);
            getsockname(svr->node.sock->fd, (struct sockaddr*)&svr->node.sock->local_addr, &len);
            ogs_freeaddrinfo(svr->node.addr);
            ogs_copyaddrinfo(&svr->node.addr, &svr->node.sock->local_addr);
            ogs_info("Ephemeral notification server(%s) [%s://%s]:%u", svr->interface ? svr->interface : "",
                     svr->ssl_ctx ? "https" : "http", OGS_ADDR(svr->node.addr, buf), OGS_PORT(svr->node.addr));
        }
    }
    return std::shared_ptr<Open5GSSBIServer>(new Open5GSSBIServer(svr));
}

std::vector <std::shared_ptr<Open5GSSBIServer> > Context::MBSFNotificationServers()
{
    return servers[MBS_NOTIFICATION_LISTENER];
}

int Context::load()
{
    return UserDataIngSession::numberOfDistributionSessions();
}

std::shared_ptr<Open5GSSBIServer> Context::getServerForAddr(const ogs_sockaddr_t *addr, int add_to_server_type)
{
    std::shared_ptr<Open5GSSBIServer> svr = findServerForAddr(addr);
    if (!svr) {
        svr = newSbiServer(addr);
        servers[add_to_server_type].push_back(svr);
    } else {
        auto it = std::find(servers[add_to_server_type].begin(), servers[add_to_server_type].end(), svr);
        if (it == servers[add_to_server_type].end()) servers[add_to_server_type].push_back(svr);
    }
    return svr;
}

const std::shared_ptr<Open5GSSBIServer> &Context::findServerForAddr(const ogs_sockaddr_t *addr) const
{
    if (addr) {
        int i = 0;
        for (i=0; i<SERVER_MAX_NUM; i++) {
            for (const auto &srv : servers[i]) {
                if (srv && ogs_sockaddr_is_equal(addr, srv->ogsSBIServer()->node.addr)) {
                    return srv;
                }
            }
        }
    }
    static const std::shared_ptr<Open5GSSBIServer>null_svr(nullptr);
    return null_svr;
}

const std::shared_ptr<Open5GSSBIServer> &Context::findServerForAddr(const ogs_socknode_t *node) const
{
    if (node) return findServerForAddr(node->addr);
    static const std::shared_ptr<Open5GSSBIServer>null_svr(nullptr);
    return null_svr;
}

bool Context::serverIsType(const Open5GSSBIServer &server, Context::ServerType typ) const
{
    if (typ >= SERVER_MAX_NUM) return false;
    for (const auto &svr : servers[typ]) {
        if (svr->ogsSBIServer() == server.ogsSBIServer()) return true;
    }
    return false;
}

const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &Context::findDistSessIdFromUrl(const std::string &notif_url) const
{
    std::lock_guard<decltype(m_notifServerMapMutex)::element_type> lock(*m_notifServerMapMutex);
    auto it = m_notifServerMap.find(notif_url);
    if (it != m_notifServerMap.end()) return it->second;
    static const std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> null_dist_sess_id;
    return null_dist_sess_id;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
