#ifndef _MBS_TF_OPEN5GS_SBI_MESSAGE_HH_
#define _MBS_TF_OPEN5GS_SBI_MESSAGE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Message interface
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

class Open5GSSBIRequest;
class Open5GSSBIResponse;

class Open5GSSBIMessage {
public:
    Open5GSSBIMessage(ogs_sbi_message_t *event, bool owner=false) :m_message(event),m_owner(owner) {};
    Open5GSSBIMessage() :m_message(nullptr),m_owner(false) {};
    Open5GSSBIMessage(Open5GSSBIMessage &&other) :m_message(other.m_message),m_owner(other.m_owner) {other.m_owner = false;};
    Open5GSSBIMessage(const Open5GSSBIMessage &other) :m_message(other.m_message),m_owner(false) {};
    Open5GSSBIMessage &operator=(Open5GSSBIMessage &&other) {m_message = other.m_message; m_owner = other.m_owner; other.m_owner = false; return *this;};
    Open5GSSBIMessage &operator=(const Open5GSSBIMessage &other) {m_message = other.m_message; m_owner = false; return *this;};
    virtual ~Open5GSSBIMessage();

    ogs_sbi_message_t *ogsSBIMessage() { return m_message; };
    const ogs_sbi_message_t *ogsSBIMessage() const { return m_message; };
    void setOwner(bool owner) { m_owner = owner; };

    void parseHeader(Open5GSSBIRequest &request);
    void parseHeader(Open5GSSBIResponse &response);

    const char *serviceName() const { return m_message?(m_message->h.service.name):nullptr; };
    const char *apiVersion() const { return m_message?(m_message->h.api.version):nullptr; };
    const char *resourceComponent(size_t idx) const;
    const char *method() const { return m_message?(m_message->h.method):nullptr; };
    const char *uri() const { return m_message?(m_message->h.uri):nullptr; };
    const char *location() const { return m_message?(m_message->http.location):nullptr; };
    const char *cacheControl() const { return m_message?(m_message->http.cache_control):nullptr; };

    const OpenAPI_nf_profile_t *nfProfile() const { return m_message?(m_message->NFProfile):nullptr; };
    void nfProfile(OpenAPI_nf_profile_t *profile) {
        if (!m_message) return;
        if (m_message->NFProfile && m_message->NFProfile != profile) OpenAPI_nf_profile_free(m_message->NFProfile);
        m_message->NFProfile = profile;
    };

    int resStatus() const { return m_message?(m_message->res_status):0; };
    void resStatus(int status) { if (!m_message) return; m_message->res_status = status; };

    const char *contentType() const { return m_message?(m_message->http.content_type):nullptr; };
    void contentType(char *ctype) {
        if (!m_message) return;
        if (m_message->http.content_type && m_message->http.content_type != ctype) ogs_free(m_message->http.content_type);
        m_message->http.content_type = ctype;
    };

    OpenAPI_problem_details_t *problemDetails() { return m_message?(m_message->ProblemDetails):nullptr; };
    const OpenAPI_problem_details_t *problemDetails() const { return m_message?(m_message->ProblemDetails):nullptr; };
    void problemDetails(OpenAPI_problem_details_t *prob_details) {
        if (!m_message) return;
        if (m_message->ProblemDetails && m_message->ProblemDetails != prob_details)
            OpenAPI_problem_details_free(m_message->ProblemDetails);
        m_message->ProblemDetails = prob_details;
    };

    operator bool() const { return !!m_message; };

private:
    ogs_sbi_message_t *m_message;
    bool m_owner;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_MESSAGE_HH_ */
