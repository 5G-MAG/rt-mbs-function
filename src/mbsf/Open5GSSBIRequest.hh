#ifndef _MBSF_OPEN5GS_SBI_REQUEST_HH_
#define _MBSF_OPEN5GS_SBI_REQUEST_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Open5GS SBI Request interface
 ******************************************************************************
 * Copyright: (C)2024-2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *            Dev Audsin <dev.audsin@bbc.co.uk>
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

#include <functional>
#include <map>
#include <optional>
#include <string>

#include "common.hh"

#include "CaseInsensitiveTraits.hh"

MBSF_NAMESPACE_START

class Open5GSSBIMessage;

class Open5GSSBIRequest {
public:
    using CaseInsensitiveString = std::basic_string<char, CaseInsensitiveTraits<char> >;
    using HeadersMap = std::map<CaseInsensitiveString, std::function<void(const CaseInsensitiveString &field, const char *val)> >;
    using ParametersMap = std::map<std::string, std::function<void(const std::string &param, const char *val)> >;

    Open5GSSBIRequest(ogs_sbi_request_t *request, bool owner = true) :m_request(request), m_owner(owner) {};
    Open5GSSBIRequest(const std::string &method, const std::string &uri, const std::string &apiVersion, const std::optional<std::string> &data, const std::optional<std::string> &type);
    Open5GSSBIRequest(Open5GSSBIMessage &message);
    Open5GSSBIRequest() = delete;
    Open5GSSBIRequest(Open5GSSBIRequest &&other) = delete;
    Open5GSSBIRequest(const Open5GSSBIRequest &other) = delete;
    Open5GSSBIRequest &operator=(Open5GSSBIRequest &&other) = delete;
    Open5GSSBIRequest &operator=(const Open5GSSBIRequest &other) = delete;
    virtual ~Open5GSSBIRequest() {if (m_owner && m_request) ogs_sbi_request_free(m_request);};

    ogs_sbi_request_t *ogsSBIRequest() { return m_request; };
    const ogs_sbi_request_t *ogsSBIRequest() const { return m_request; };

    operator bool() const { return !!m_request; };

    std::string headerValue(const std::string &field, const std::string &defval = std::string()) const;
    std::string parameterValue(const std::string &field, const std::string &defval = std::string()) const;
    void headersMap(const HeadersMap &map) const;
    void parametersMap(const ParametersMap &map) const;

    const char *serviceName() const { return m_request?(m_request->h.service.name):nullptr; };
    const char *apiVersion() const { return m_request?(m_request->h.api.version):nullptr; };
    const char *resourceComponent(size_t idx) const;
    const char *method() const { return m_request?(m_request->h.method):nullptr; };
    const char *content() const { return m_request?m_request->http.content:nullptr; };
    size_t contentLength() const { return m_request?m_request->http.content_length:0; };
    const char *uri() const { return m_request?m_request->h.uri:nullptr; };
    void setOwner(bool owner) { m_owner = owner; };
    bool getOwner() const { return m_owner; };

private:
    ogs_sbi_request_t *m_request;
    bool m_owner;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OPEN5GS_SBI_REQUEST_HH_ */
