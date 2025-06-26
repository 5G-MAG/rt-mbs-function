#ifndef _MBS_TF_OPEN5GS_SBI_RESPONSE_HH_
#define _MBS_TF_OPEN5GS_SBI_RESPONSE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Response interface
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
#include "ogs-sbi.h"

#include <memory>

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSBIResponse {
public:
    Open5GSSBIResponse(ogs_sbi_response_t *response, bool owner = true) :m_response(response), m_owner(owner) {};
    Open5GSSBIResponse(const Open5GSSBIResponse &other) :m_response(other.m_response), m_owner(false) {};
    Open5GSSBIResponse(Open5GSSBIResponse &&other) :m_response(other.m_response), m_owner(other.m_owner) {
        other.m_owner = false;
    };
    Open5GSSBIResponse() = delete;
    Open5GSSBIResponse &operator=(Open5GSSBIResponse &&other) { m_response = other.m_response; m_owner = other.m_owner; other.m_owner = false; return *this; };
    Open5GSSBIResponse &operator=(const Open5GSSBIResponse &other) { m_response = other.m_response; m_owner = false; return *this; };
    virtual ~Open5GSSBIResponse() { if (m_owner && m_response) ogs_sbi_response_free(m_response); };

    ogs_sbi_response_t *ogsSBIResponse() { return m_response; };
    const ogs_sbi_response_t *ogsSBIResponse() const { return m_response; };

    int status() const { return m_response?m_response->status:0; };
    Open5GSSBIResponse &status(int respcode) { if (m_response) m_response->status = respcode; return *this; };
    size_t contentLength() const { return m_response?m_response->http.content_length:0; };
    Open5GSSBIResponse &contentLength(size_t length) { if (m_response) m_response->http.content_length = length; return *this; };
    const char *content() const { return m_response?m_response->http.content:nullptr; };
    Open5GSSBIResponse &content(std::string &content) { if (m_response) m_response->http.content = content.data(); return *this; };
    Open5GSSBIResponse &content(char *content) { if (m_response) m_response->http.content = content; return *this; };
    const char *getHeader(const char *header);
    bool headerSet(const std::string &field, const std::string &value);
    Open5GSSBIResponse &setOwner(bool owner) { m_owner = owner; return *this; };

private:
    ogs_sbi_response_t *m_response;
    bool m_owner;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_RESPONSE_HH_ */
