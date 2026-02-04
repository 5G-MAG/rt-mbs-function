/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Associated Session Id class
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
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <optional>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "MBSMFMBSSession.hh"
#include "openapi/model/AssociatedSessionId.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "AssociatedSessId.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::AssociatedSessionId;
using reftools::mbsf::IpAddr;

MBSF_NAMESPACE_START

static bool resolve_src_dest_addr(const std::string &src_ipv4_addr, const std::string &dest_ipv4_addr, struct addrinfo **ai_src, struct addrinfo **ai_dest);
static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr);

AssociatedSessId::AssociatedSessId(CJson &json, bool as_request)
    :m_associatedSessionId(new AssociatedSessionId(json, as_request))
{
}

AssociatedSessId::AssociatedSessId(const std::shared_ptr<AssociatedSessionId> &associated_session_id)
    :m_associatedSessionId(associated_session_id)
{
}

AssociatedSessId::~AssociatedSessId()
{
}

CJson AssociatedSessId::json(bool as_request = false) const
{
    return m_associatedSessionId->toJSON(as_request);
}

#if 0
typedef struct mb_smf_sc_ssm_addr_s {
    int family;               /**< The AF family for the SSM address, either AF_INET or AF_INET6. */
    /** SSM Source Address
     * The source address for the SSM using the address family given by the @ref mb_smf_sc_ssm_addr_s::family "family" member.
     */
    union {
        struct in_addr ipv4;  /**< The IPv4 source address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET. */
        struct in6_addr ipv6; /**< The IPv6 source address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET6. */
    } source;
    /** SSM Destination Address
     * The destination address for the SSM using the address family given by the @ref mb_smf_sc_ssm_addr_s::family "family" member.
     */
    union {
        struct in_addr ipv4;  /**< The IPv4 destination address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET. */
        struct in6_addr ipv6; /**< The IPv6 destination address if @ref mb_smf_sc_ssm_addr_s::family "family" is AF_INET6. */
    } dest_mc;
} mb_smf_sc_ssm_addr_t;


#endif

mb_smf_sc_associated_session_id_t *AssociatedSessId::populateAssociatedSessionId() {

    mb_smf_sc_associated_session_id_t *session_id = mb_smf_sc_associated_session_id_new();
    std::memset(&session_id->ssm, 0, sizeof(session_id->ssm));
    populateSsm(session_id);
    return session_id;
}

void AssociatedSessId::populateSsm(mb_smf_sc_associated_session_id_t *session_id) {

    std::shared_ptr< IpAddr > src_ip_addr = getSourceIpAddr();
    std::optional<std::string > src_ipv4_addr = src_ip_addr->getIpv4Addr();
    std::optional<std::shared_ptr< std::string > > src_ipv6_addr = src_ip_addr->getIpv6Addr();

    std::shared_ptr< IpAddr > dest_ip_addr = getDestIpAddr();
    std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
    std::optional<std::shared_ptr< std::string > > dest_ipv6_addr = dest_ip_addr->getIpv6Addr();

    if(!src_ip_addr && !dest_ip_addr) {
        return;
    } else if (src_ipv4_addr.has_value() && dest_ipv4_addr.has_value()) {
        struct addrinfo *ai_src = NULL, *ai_dest = NULL;
        void *src_addr = NULL, *dest_addr = NULL;

        if(resolve_src_dest_addr(src_ipv4_addr.value(), dest_ipv4_addr.value(), &ai_src, &ai_dest))
        {
            if(get_src_dest_of_same_addr_family(AF_INET, ai_src, ai_dest, &src_addr, &dest_addr))
            {
                session_id->ssm.family = AF_INET;
                if (src_addr != nullptr) {
                    const struct in_addr *src_in = reinterpret_cast<const struct in_addr *>(src_addr);
                    session_id->ssm.source.ipv4 = *src_in;
                }

                if (dest_addr != nullptr) {
                    const struct in_addr *dest_in = reinterpret_cast<const struct in_addr *>(dest_addr);
                    session_id->ssm.dest_mc.ipv4 = *dest_in;
                }

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv4 address family");
               if(ai_src) {
                   freeaddrinfo(ai_src);
                   ai_src = NULL;
               }

               if(ai_dest) {
                   freeaddrinfo(ai_dest);
                   ai_dest = NULL;
                }
                return;
            }
        }
        if(ai_src) {
            freeaddrinfo(ai_src);
            ai_src = NULL;
        }

        if(ai_dest) {
            freeaddrinfo(ai_dest);
            ai_dest = NULL;
        }

    } else if (src_ipv6_addr.has_value() && dest_ipv6_addr.has_value()) {
       struct addrinfo *ai_src = NULL, *ai_dest = NULL;
       void *src_addr = NULL, *dest_addr = NULL;

       std::shared_ptr< std::string >  src_ipv6 = src_ipv6_addr.value();
       std::shared_ptr< std::string >  dest_ipv6 = dest_ipv6_addr.value();

       if(resolve_src_dest_addr(*src_ipv6, *dest_ipv6, &ai_src, &ai_dest))
       {
           if(get_src_dest_of_same_addr_family(AF_INET6, ai_src, ai_dest, &src_addr, &dest_addr))
           {
               session_id->ssm.family = AF_INET6;
                if (src_addr != nullptr) {
                    const struct in6_addr *src_in = reinterpret_cast<const struct in6_addr *>(src_addr);
                    session_id->ssm.source.ipv6 = *src_in;
                }

                if (dest_addr != nullptr) {
                    const struct in6_addr *dest_in = reinterpret_cast<const struct in6_addr *>(dest_addr);
                    session_id->ssm.dest_mc.ipv6 = *dest_in;
                }


           } else {
               ogs_error("Unable to resolve SSM addresses for IPv6 address family");
               if(ai_src) freeaddrinfo(ai_src);
               if(ai_dest) freeaddrinfo(ai_dest);
               return;
           }
       }
       if(ai_src) freeaddrinfo(ai_src);
       if(ai_dest) freeaddrinfo(ai_dest);

    } else {
       ogs_error("Unable to resolve SSM addresses");
    }
}


static bool resolve_src_dest_addr(const std::string &src_addr, const std::string &dest_addr, struct addrinfo **src_addrinfo, struct addrinfo **dest_addrinfo)
{
        ogs_debug("resolving SSM");
        int result;

        result = getaddrinfo(src_addr.c_str(), NULL, NULL, src_addrinfo);
        if (result) {
            ogs_error("Unable to resolve SSM source address '%s': %s", src_addr.c_str(), gai_strerror(result));
            if (src_addrinfo && *src_addrinfo) {
                freeaddrinfo(*src_addrinfo);
                *src_addrinfo = NULL;
            }
            return false;
        }

        result = getaddrinfo(dest_addr.c_str(), NULL, NULL, dest_addrinfo);
        if (result) {
            ogs_error("Unable to resolve SSM multicast destination address '%s': %s", dest_addr.c_str(), gai_strerror(result));
            if (src_addrinfo && *src_addrinfo) {
                freeaddrinfo(*src_addrinfo);
                *src_addrinfo = NULL;
            }
            if (dest_addrinfo && *dest_addrinfo) {
                freeaddrinfo(*dest_addrinfo);
                *dest_addrinfo = NULL;
            }
            return false;
        }
        return true;

}

static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr)
{
    while (src_addrinfo && src_addrinfo->ai_family != family) src_addrinfo = src_addrinfo->ai_next;
    if (!src_addrinfo) return false;

    while (dest_addrinfo && dest_addrinfo->ai_family != family) dest_addrinfo = dest_addrinfo->ai_next;
    if (!dest_addrinfo) return false;

    if (family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in*)src_addrinfo->ai_addr;
        *src_addr = &addr->sin_addr;
        addr = (struct sockaddr_in*)dest_addrinfo->ai_addr;
        *dest_addr =  &addr->sin_addr;
    } else if (family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6*)src_addrinfo->ai_addr;
        *src_addr = &addr->sin6_addr;
        addr = (struct sockaddr_in6*)dest_addrinfo->ai_addr;
        *dest_addr = &addr->sin6_addr;
    } else {
        *src_addr = NULL;
        *dest_addr = NULL;
    }
    return true;
}



MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

