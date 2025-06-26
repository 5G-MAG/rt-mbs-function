#ifndef _MBS_TF_OPEN5GS_SBI_CLIENT_HH_
#define _MBS_TF_OPEN5GS_SBI_CLIENT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Client interface
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

class Open5GSSBIClient {
public:
    Open5GSSBIClient();
    Open5GSSBIClient(ogs_sbi_client_t *client);
    Open5GSSBIClient(const char *hostname, int port);
    Open5GSSBIClient(const std::string &url);
    Open5GSSBIClient(Open5GSSBIClient &&other) = delete;
    Open5GSSBIClient(const Open5GSSBIClient &other) = delete;
    Open5GSSBIClient &operator=(Open5GSSBIClient &&other) = delete;
    Open5GSSBIClient &operator=(const Open5GSSBIClient &other) = delete;
    virtual ~Open5GSSBIClient();

    ogs_sockaddr_t *ogsSockaddr(std::shared_ptr<Open5GSSBIClient> &client);
    ogs_sbi_client_t *ogsSBIClient() { return m_ogsClient; };

    /*static*/ bool sendRequest(ogs_sbi_client_cb_f client_notify_cb,  std::shared_ptr<Open5GSSBIRequest> request, void *data);

    operator bool() const { return !!m_ogsClient; };

private:
    ogs_sbi_client_t *m_ogsClient;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_CLIENT_HH_ */

