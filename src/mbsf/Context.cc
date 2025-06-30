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
#include <stdexcept>
#include "ogs-sbi.h"
#include "ogs-app.h"

#include "common.hh"
#include "App.hh"
#include "Open5GSNetworkFunction.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSockAddr.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSYamlIter.hh"
#include "UserService.hh"

#include "Context.hh"

MBSF_NAMESPACE_START

Context::Context()
    :servers()
    ,cacheControl({60, 60})
    ,capacity({100,100})	
{
}

Context::~Context()
{
    for (auto &svrs : servers) {
        for (auto &svr: svrs) {
            svr.reset();
        }
    }
}

bool Context::parseConfig()
{
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
                } else if (mbsf_key == "mbsUserServices" || mbsf_key == "mbsUserDataIngestSession" ) {
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
                            throw std::out_of_range("Bad configuration node at mbsf.mbsUserServices");
                        }

                    } while (mbsUserServices_array.type() == YAML_SEQUENCE_NODE);

                } else {
                    ogs_warn("Unknown key `mbsf.%s` in configuration", mbsf_key.c_str());
                }
            }
        }
    }

    //if (!validate()) {
    //    return false;
    //}

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

void Context::parseCacheControl(Open5GSYamlIter &iter) {
    while (iter.next()) {
        std::string cc_key(iter.key());
        std::string cc_val(iter.value());
        try {
            if (cc_key == "maxAge") {
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

void Context::parseConfiguration(std::string &pc_key, Open5GSYamlIter &iter)   {
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
             /*
	     rv = ogs_app_config_parse_sockopt(&iter, &option);
             if (rv != OGS_OK) {
                 ogs_debug("ogs_app_config_parse_sockopt() failed");
                 return rv;
             }
	     */
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
        if (ogs_app()->parameter.no_ipv4 == 0)
            ogs_socknode_add(&list, AF_INET, addr, NULL);
        if (ogs_app()->parameter.no_ipv6 == 0)
            ogs_socknode_add(&list6, AF_INET6, addr, NULL);
	ogs_freeaddrinfo(addr);
    }

    if (dev) {
        rv = ogs_socknode_probe(
			ogs_app()->parameter.no_ipv4 ? NULL : &list,
                                    ogs_app()->parameter.no_ipv6 ? NULL : &list6,
                                    dev, port, NULL);
        ogs_assert(rv == OGS_OK);
    }

    addr = NULL;
    for (i = 0; i < num_of_advertise; i++) {
        rv = ogs_addaddrinfo(&addr,
                                    family, advertise[i], port, 0);
        ogs_assert(rv == OGS_OK);
    }
    node = (ogs_socknode_t *)ogs_list_first(&list);
    if (node) {
        int matches = 0;
        //ogs_sbi_server_t *server;
	
	matches = checkForAddr(node);

	if(!matches) {
           		
	    std::shared_ptr<Open5GSSBIServer> new_server;
            new_server.reset(new Open5GSSBIServer(node, is_option ? &option : nullptr));
            new_server->ogsSBIServerAdvertise(addr);

            if (pc_key == "mbsUserServices") {
                servers[MBS_USER_SERVICES].push_back(new_server);
            } else if (pc_key == "mbsUserDataIngestSession") {
                 servers[MBS_USER_DATA_INGEST_SESSION].push_back(new_server);
            }

            /*
                if (key) server->tls.key = key;
                if (pem) server->tls.pem = pem;
            */
        }
    }
    node6 = (ogs_socknode_t *)ogs_list_first(&list6);
    if (node6) {
        int matches = 0;

	matches = checkForAddr(node);

        if(!matches) {
	    	
	    std::shared_ptr<Open5GSSBIServer> new_server;
            new_server.reset(new Open5GSSBIServer(node, is_option ? &option : nullptr));
            new_server->ogsSBIServerAdvertise(addr);
            
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
            std::shared_ptr<Open5GSSockAddr> sockAddr;
            sockAddr.reset(new Open5GSSockAddr(srv->ogsSBIServer()->node.addr));
            sockAddrs.push_back(sockAddr);
        }
    }
    return sockAddrs;
}

std::vector <std::shared_ptr<Open5GSSockAddr> > Context::MBSFUserDataIngestSessionAddresses()
{
    std::vector<std::shared_ptr<Open5GSSockAddr> > sockAddrs;
    std::vector<std::shared_ptr<Open5GSSBIServer>> srvs = servers[MBS_USER_DATA_INGEST_SESSION];
    if(!srvs.empty()) {
        for (const auto &srv: srvs) {
            std::shared_ptr<Open5GSSockAddr> sockAddr;
            sockAddr.reset(new Open5GSSockAddr(srv->ogsSBIServer()->node.addr));
            sockAddrs.push_back(sockAddr);
        }
    }
    return sockAddrs;
}

int Context::checkForAddr(ogs_socknode_t *node)
{
    int i = 0;
    for (i=0; i<SERVER_MAX_NUM; i++) {
        std::vector<std::shared_ptr<Open5GSSBIServer>> srvs = servers[i];
        for (const auto &srv: srvs) {
            if( srv && ogs_sockaddr_is_equal(node->addr, srv->ogsSBIServer()->node.addr)) {
                return 1;
            }
        }
    }
    return 0;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
