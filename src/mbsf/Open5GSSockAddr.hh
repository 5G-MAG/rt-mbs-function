#ifndef _MBS_TF_OPEN5GS_SBI_SOCKADDR_HH_
#define _MBS_TF_OPEN5GS_SBI_SOCKADDR_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI SockAddr interface
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

#include "ogs-proto.h"
#include "ogs-core.h"

#include <memory>

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSockAddr {
public:
    Open5GSSockAddr(ogs_sockaddr_t *sockaddr) :m_sockaddr(sockaddr) {};
    Open5GSSockAddr() = delete;
    Open5GSSockAddr(Open5GSSockAddr &&other) = delete;
    Open5GSSockAddr(const Open5GSSockAddr &other) = delete;
    Open5GSSockAddr &operator=(Open5GSSockAddr &&other) = delete;
    Open5GSSockAddr &operator=(const Open5GSSockAddr &other) = delete;
    virtual ~Open5GSSockAddr() {};

    ogs_sockaddr_t *ogsSockAddr() { return m_sockaddr; };
    const ogs_sockaddr_t *ogsSockAddr() const { return m_sockaddr; };

private:
    ogs_sockaddr_t *m_sockaddr;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_SOCKADDR_HH_ */
