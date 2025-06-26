#ifndef _MBS_TF_OPEN5GS_SBI_REQUEST_HH_
#define _MBS_TF_OPEN5GS_SBI_REQUEST_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Request interface
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

#include <string>
#include <optional>

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSBIRequest {
public:
    Open5GSSBIRequest(ogs_sbi_request_t *request, bool owner = true) :m_request(request), m_owner(owner) {};
    Open5GSSBIRequest(const std::string &method, const std::string &uri, const std::string &apiVersion, const std::optional<std::string> &data, const std::optional<std::string> &type);
    Open5GSSBIRequest() = delete;
    Open5GSSBIRequest(Open5GSSBIRequest &&other) = delete;
    Open5GSSBIRequest(const Open5GSSBIRequest &other) = delete;
    Open5GSSBIRequest &operator=(Open5GSSBIRequest &&other) = delete;
    Open5GSSBIRequest &operator=(const Open5GSSBIRequest &other) = delete;
    virtual ~Open5GSSBIRequest() {if (m_owner) ogs_sbi_request_free(m_request);};

    ogs_sbi_request_t *ogsSBIRequest() { return m_request; };
    const ogs_sbi_request_t *ogsSBIRequest() const { return m_request; };

    operator bool() const { return !!m_request; };

    std::string headerValue(const std::string &field, const std::string &defval = std::string()) const;

    const char *content() const { return m_request?m_request->http.content:nullptr; };
    const char *uri() const { return m_request?m_request->h.uri:nullptr; };
    void setOwner(bool owner) { m_owner = owner; };

private:
    ogs_sbi_request_t *m_request;
    bool m_owner;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_REQUEST_HH_ */
