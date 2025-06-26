#ifndef _MBS_TF_OPEN5GS_SBI_SERVER_HH_
#define _MBS_TF_OPEN5GS_SBI_SERVER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Server interface
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

class Open5GSSBIServer {
public:
    Open5GSSBIServer();
    Open5GSSBIServer(ogs_sbi_server_t *server);
    Open5GSSBIServer(ogs_socknode_t *node,  ogs_sockopt_t *option);
    Open5GSSBIServer(Open5GSSBIServer &&other) = delete;
    Open5GSSBIServer(const Open5GSSBIServer &other) = delete;
    Open5GSSBIServer &operator=(Open5GSSBIServer &&other) = delete;
    Open5GSSBIServer &operator=(const Open5GSSBIServer &other) = delete;
    virtual ~Open5GSSBIServer();

    ogs_sockaddr_t *ogsSockaddr(std::shared_ptr<Open5GSSBIServer> &server);
    void ogsSBIServerAdvertise(ogs_sockaddr_t *addr);
    ogs_sbi_server_t *ogsSBIServer() { return m_ogsServer; };

    static bool sendError(Open5GSSBIStream &stream, int status_code, std::optional<Open5GSSBIMessage> message, const char *reason, const char *value);
    static bool sendResponse(Open5GSSBIStream &stream, Open5GSSBIResponse &response);

    operator bool() const { return !!m_ogsServer; };

private:
    ogs_sbi_server_t *m_ogsServer;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_SERVER_HH_ */
