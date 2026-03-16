/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Client class
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
#include "ogs-sbi.h"

#include <memory>
#include <stdexcept>

#include "common.hh"
#include "App.hh"
#include "TimerFunc.hh"
#include "Open5GSEvent.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSSBIStream.hh"

#include "Open5GSSBIClient.hh"

MBSF_NAMESPACE_START

Open5GSSBIClient::Open5GSSBIClient()
{
}

Open5GSSBIClient::Open5GSSBIClient(ogs_sbi_client_t *client)
    :m_ogsClient(client)
{
}

Open5GSSBIClient::Open5GSSBIClient(const std::string &url)
{

    OpenAPI_uri_scheme_e scheme = OpenAPI_uri_scheme_NULL;
    ogs_sockaddr_t *addr = NULL;
    ogs_sockaddr_t *addr6 = NULL;
    char *fqdn = NULL;
    uint16_t fqdn_port = 0;
    bool rc;

    rc = ogs_sbi_getaddr_from_uri(&scheme, &fqdn, &fqdn_port, &addr, &addr6, (char *)url.c_str());
    if (rc == false || scheme == OpenAPI_uri_scheme_NULL) {
         ogs_error("Failed to get sockaddr from uri");
         throw std::runtime_error("Failed to get sockaddr from uri!");
     }
     m_ogsClient = ogs_sbi_client_add(scheme, fqdn, fqdn_port, addr, addr6);
     ogs_assert(m_ogsClient);
     if (fqdn) ogs_free(fqdn);
     if (addr) ogs_freeaddrinfo(addr);
     if (addr6) ogs_freeaddrinfo(addr6);
}

Open5GSSBIClient::Open5GSSBIClient(const char *hostname, int port)
{
    ogs_sockaddr_t *addr = NULL;
    OpenAPI_uri_scheme_e scheme = OpenAPI_uri_scheme_http;
    int rv;

    rv = ogs_getaddrinfo(&addr, AF_UNSPEC, hostname, port, 0);
    if (rv != OGS_OK) {
        ogs_error("getaddrinfo failed");
        throw std::runtime_error("getaddrinfo failed!");
    }

    if (addr == nullptr) {
        ogs_error("Could not get the address of the Application Function");
        throw std::runtime_error("Unable to get client address!");
    }

    m_ogsClient = ogs_sbi_client_add(scheme, const_cast<char*>(hostname), port,
                                     (addr->ogs_sa_family == AF_INET)?addr:NULL, (addr->ogs_sa_family == AF_INET6)?addr:NULL);
    ogs_assert(m_ogsClient);

    if (addr) ogs_freeaddrinfo(addr);
}

Open5GSSBIClient::~Open5GSSBIClient()
{
}

bool Open5GSSBIClient::sendRequest(ogs_sbi_client_cb_f client_notify_cb,  std::shared_ptr<Open5GSSBIRequest> request, void *data)
{
    ogs_sbi_client_send_request(this->ogsSBIClient(), client_notify_cb, request->ogsSBIRequest(), (void *)data);
    //ogs_sbi_request_free(request->ogsSBIRequest());

    return 1;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
