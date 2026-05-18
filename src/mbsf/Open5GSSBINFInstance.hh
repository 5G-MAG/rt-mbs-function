#ifndef _MBSF_OPEN5GS_SBI_NF_INSTANCE_HH_
#define _MBSF_OPEN5GS_SBI_NF_INSTANCE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Open5GS SBI NF Instance interface
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

#include "ogs-sbi.h"

#include <memory>
#include <optional>
#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSBIStream;
class Open5GSSBIMessage;
class Open5GSSBIObject;


class Open5GSSBINFInstance {
public:


    Open5GSSBINFInstance() = delete;
    Open5GSSBINFInstance(ogs_sbi_nf_instance_t *instance, bool owner = false);
    Open5GSSBINFInstance(const std::string &id, bool owner = false);
    Open5GSSBINFInstance(Open5GSSBINFInstance &&other) = delete;
    Open5GSSBINFInstance(const Open5GSSBINFInstance &other) = delete;
    Open5GSSBINFInstance &operator=(Open5GSSBINFInstance &&other) = delete;
    Open5GSSBINFInstance &operator=(const Open5GSSBINFInstance &other) = delete;
    virtual ~Open5GSSBINFInstance();

    ogs_sbi_nf_instance_t *ogsSBINFInstance() { return m_ogsSbiNfInstance; };

    ogs_sockaddr_t **Ipv4() {return m_ogsSbiNfInstance->ipv4;};
    int numberOfIPv4() { return m_ogsSbiNfInstance->num_of_ipv4;};

    ogs_sockaddr_t **Ipv6() {return m_ogsSbiNfInstance->ipv6;};
    int numberOfIPv6() { return m_ogsSbiNfInstance->num_of_ipv6;};

    void setOwner(bool owner) { m_owner = owner; };
    bool getOwner() const { return m_owner; };

    operator bool() const { return !!m_ogsSbiNfInstance; };

private:
    ogs_sbi_nf_instance_t *m_ogsSbiNfInstance;
    bool m_owner;

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OPEN5GS_SBI_OBJECT_HH_ */

