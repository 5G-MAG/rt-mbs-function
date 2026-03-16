/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Open5GS SBI Message interface
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

#include "ogs-app.h"
#include "ogs-proto.h"

#include <stdexcept>
#include <memory>
#include <cstring>

#include "common.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"

#include "Open5GSSBIMessage.hh"

MBSF_NAMESPACE_START

static void clear_header(ogs_sbi_header_t *header);

Open5GSSBIMessage::~Open5GSSBIMessage()
{
    //ogs_debug("~Open5GSSBIMessage: message=%p, owner=%s", m_message, m_owner?"true":"false");
    if (m_message != nullptr && m_owner) {
        ogs_sbi_message_free(m_message);
        delete m_message;
        m_message = nullptr;
    }
}

void Open5GSSBIMessage::parseHeader(Open5GSSBIRequest &request)
{
    ensureMessage();
    resetHeader();
    clear_header(&request.ogsSBIRequest()->h);
    int rv = ogs_sbi_parse_header(m_message, &request.ogsSBIRequest()->h);
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_header() failed");
    m_parsedHeader = &request.ogsSBIRequest()->h;

    bool discovery_option_presence = false;
    ogs_sbi_discovery_option_t *discovery_option = ogs_sbi_discovery_option_new();

    Open5GSSBIRequest::ParametersMap params_map = {
        { OGS_SBI_PARAM_TARGET_NF_TYPE, [this](const std::string &key, const char *val) -> void {
                                            char *pval = const_cast<char*>(val);
                                            m_message->param.target_nf_type = OpenAPI_nf_type_FromString(pval); } },
        { OGS_SBI_PARAM_REQUESTER_NF_TYPE, [this](const std::string &key, const char *val) {
                                            char *pval = const_cast<char*>(val);
                                            m_message->param.requester_nf_type = OpenAPI_nf_type_FromString(pval); } },
        { OGS_SBI_PARAM_TARGET_NF_INSTANCE_ID, [this, &discovery_option, &discovery_option_presence](const std::string &key, const char *val) {
                                            if (val) {
                                                char *pval = const_cast<char*>(val);
                                                ogs_sbi_discovery_option_set_target_nf_instance_id(discovery_option, pval);
                                                discovery_option_presence = true;
                                            } } },
        { OGS_SBI_PARAM_REQUESTER_NF_INSTANCE_ID, [this, &discovery_option, &discovery_option_presence](const std::string &key, const char *val) {
                                            if (val) {
                                                char *pval = const_cast<char*>(val);
                                                ogs_sbi_discovery_option_set_requester_nf_instance_id(discovery_option, pval);
                                                discovery_option_presence = true;
                                            } } },
        { OGS_SBI_PARAM_SERVICE_NAMES, [this, &discovery_option, &discovery_option_presence](const std::string &key, const char *val) {
                                            if (val) {
                                                char *pval = const_cast<char*>(val);
                                                ogs_sbi_discovery_option_parse_service_names(discovery_option, pval);
                                                discovery_option_presence = true;
                                            } } },
        { OGS_SBI_PARAM_REQUESTER_FEATURES, [this, &discovery_option, &discovery_option_presence](const std::string &key, const char *val) {
                                            if (val) {
                                                char *pval = const_cast<char*>(val);
                                                discovery_option->requester_features = ogs_uint64_from_string(pval);
                                                discovery_option_presence = true;
                                            } } },
        { OGS_SBI_PARAM_NF_ID, [this](const std::string &key, const char *val) {
                                            m_message->param.nf_id = const_cast<char*>(val); } },
        { OGS_SBI_PARAM_NF_TYPE, [this](const std::string &key, const char *val) {
                                            m_message->param.nf_type = OpenAPI_nf_type_FromString(const_cast<char*>(val)); } },
        { OGS_SBI_PARAM_LIMIT, [this](const std::string &key, const char *val) {
                                            m_message->param.limit = atoi(val); } },
        { OGS_SBI_PARAM_DNN, [this](const std::string &key, const char *val) {
                                            m_message->param.dnn = const_cast<char*>(val); } },
        { OGS_SBI_PARAM_PLMN_ID, [this](const std::string &key, const char *val) {
                                            if (val) {
                                                cJSON *item = cJSON_Parse(val);
                                                if (item) {
                                                    OpenAPI_plmn_id_t *plmn_id = OpenAPI_plmn_id_parseFromJSON(item);
                                                    if (plmn_id && plmn_id->mnc && plmn_id->mcc) {
                                                        ogs_plmn_id_build(&m_message->param.plmn_id, atoi(plmn_id->mcc),
                                                                            atoi(plmn_id->mnc), strlen(plmn_id->mnc));
                                                        m_message->param.plmn_id_presence = true;
                                                        OpenAPI_plmn_id_free(plmn_id);
                                                    }
                                                    cJSON_Delete(item);
                                                }
                                            } } },
        { OGS_SBI_PARAM_SINGLE_NSSAI, [this](const std::string &key, const char *val) {
                                            if (val) {
                                                char *pval = const_cast<char*>(val);
                                                bool rc = ogs_sbi_s_nssai_from_string(&m_message->param.s_nssai, pval);
                                                if (rc == true) m_message->param.single_nssai_presence = true;
                                            } } },
        { OGS_SBI_PARAM_SNSSAI, [this](const std::string &key, const char *val) {
                                            if (val) {
                                                char *pval = const_cast<char*>(val);
                                                bool rc = ogs_sbi_s_nssai_from_string(&m_message->param.s_nssai, pval);
                                                if (rc == true) m_message->param.snssai_presence = true;
                                            } } },
        { OGS_SBI_PARAM_SLICE_INFO_REQUEST_FOR_PDU_SESSION, [this](const std::string &key, const char *val) {
                                            if (val) {
                                                cJSON *item = cJSON_Parse(val);
                                                if (item) {
                                                    OpenAPI_slice_info_for_pdu_session_t *SliceInfoForPduSession =
                                                                            OpenAPI_slice_info_for_pdu_session_parseFromJSON(item);
                                                    if (SliceInfoForPduSession) {
                                                        OpenAPI_snssai_t *s_nssai = SliceInfoForPduSession->s_nssai;
                                                        if (s_nssai) {
                                                            m_message->param.s_nssai.sst = s_nssai->sst;
                                                            m_message->param.s_nssai.sd = ogs_s_nssai_sd_from_string(s_nssai->sd);
                                                        }
                                                        m_message->param.roaming_indication =
                                                                                        SliceInfoForPduSession->roaming_indication;
                                                        m_message->param.slice_info_request_for_pdu_session_presence = true;
                                                        OpenAPI_slice_info_for_pdu_session_free(SliceInfoForPduSession);
                                                    }
                                                    cJSON_Delete(item);
                                                }
                                            } } },
        { OGS_SBI_PARAM_IPV4ADDR, [this](const std::string &key, const char *val) {
                                            m_message->param.ipv4addr = const_cast<char*>(val); } },
        { OGS_SBI_PARAM_IPV6PREFIX, [this](const std::string &key, const char *val) {
                                            m_message->param.ipv6prefix = const_cast<char*>(val); } },
    };
    request.parametersMap(params_map);

    if (discovery_option_presence == true)
        /* message->param.discovery_option will be freed()
         * in ogs_sbi_message_free() */
        m_message->param.discovery_option = discovery_option;
    else if (discovery_option)
        ogs_sbi_discovery_option_free(discovery_option);

    Open5GSSBIRequest::HeadersMap hdrs_map = {
        { OGS_SBI_ACCEPT_ENCODING, [this](const auto &key, const char *val) {
                                        m_message->http.content_encoding = const_cast<char*>(val); } },
        { OGS_SBI_CONTENT_TYPE, [this](const auto &key, const char *val) {
                                        m_message->http.content_type = const_cast<char*>(val); } },
        { OGS_SBI_ACCEPT, [this](const auto &key, const char *val) {
                                        m_message->http.accept = const_cast<char*>(val); } },
        { OGS_SBI_CUSTOM_CALLBACK, [this](const auto &key, const char *val) {
                                        m_message->http.custom.callback = const_cast<char*>(val); } },
    };
    request.headersMap(hdrs_map);
}

void Open5GSSBIMessage::parseHeader(Open5GSSBIResponse &response)
{
    ensureMessage();
    resetHeader();


    clear_header(&response.ogsSBIResponse()->h);
    int rv = ogs_sbi_parse_header(m_message, &response.ogsSBIResponse()->h);
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_header() failed");
    m_parsedHeader = &response.ogsSBIResponse()->h;

    Open5GSSBIResponse::HeadersMap hdrs_map = {
        { OGS_SBI_CONTENT_TYPE, [this](const auto &key, const char *val) {
                                        m_message->http.content_type = const_cast<char*>(val); } },
        { OGS_SBI_LOCATION, [this](const auto &key, const char *val) {
                                        m_message->http.location = const_cast<char*>(val); } },
    };
    response.headersMap(hdrs_map);

    m_message->res_status = response.ogsSBIResponse()->status;
}

void Open5GSSBIMessage::parseResponse(Open5GSSBIResponse &response)
{
    ensureMessage();
    resetHeader();
    clear_header(&response.ogsSBIResponse()->h);
    int rv = ogs_sbi_parse_response(m_message, response.ogsSBIResponse());
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_response() failed");
    m_parsedHeader = &response.ogsSBIResponse()->h;
}

void Open5GSSBIMessage::parseRequest(Open5GSSBIRequest &request)
{
    ensureMessage();
    resetHeader();
    clear_header(&request.ogsSBIRequest()->h);
    int rv = ogs_sbi_parse_request(m_message, request.ogsSBIRequest());
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_request() failed");
    m_parsedHeader = &request.ogsSBIRequest()->h;

}

const char *Open5GSSBIMessage::resourceComponent(size_t idx) const
{
    if (!m_message) return nullptr;
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) return nullptr;
    return m_message->h.resource.component[idx];
}

Open5GSSBIMessage &Open5GSSBIMessage::resourceComponent(size_t idx, char *resource_component)
{
    ensureMessage();
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) throw std::runtime_error("Maximum number of resource component failed");
    m_message->h.resource.component[idx] = resource_component;
    return *this;
}

Open5GSSBIMessage &Open5GSSBIMessage::method(char *method)
{
    ensureMessage();
    m_message->h.method = method;
    return *this;
};

Open5GSSBIMessage &Open5GSSBIMessage::serviceName(char *service_name)
{
    ensureMessage();
    m_message->h.service.name = service_name;
    return *this;
};

Open5GSSBIMessage &Open5GSSBIMessage::apiVersion(char *api_version)
{

    ensureMessage();
    m_message->h.api.version = api_version;
    return *this;
};

Open5GSSBIMessage &Open5GSSBIMessage::nfProfile(OpenAPI_nf_profile_t *profile)
{
    ensureMessage();
    if (m_message->NFProfile && m_message->NFProfile != profile) OpenAPI_nf_profile_free(m_message->NFProfile);
    m_message->NFProfile = profile;
    return *this;
}

Open5GSSBIMessage &Open5GSSBIMessage::resStatus(int status)
{
    ensureMessage();
    m_message->res_status = status;
    return *this;
}

Open5GSSBIMessage &Open5GSSBIMessage::contentType(char *ctype)
{
    ensureMessage();
    //if (m_message->http.content_type && m_message->http.content_type != ctype) ogs_free(m_message->http.content_type);
    m_message->http.content_type = ctype;
    return *this;
}

Open5GSSBIMessage &Open5GSSBIMessage::problemDetails(OpenAPI_problem_details_t *prob_details) {
    ensureMessage();
    if (m_message->ProblemDetails && m_message->ProblemDetails != prob_details)
            OpenAPI_problem_details_free(m_message->ProblemDetails);
    m_message->ProblemDetails = prob_details;
    return *this;
}

/*** private: ***/

void Open5GSSBIMessage::ensureMessage()
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
}

void Open5GSSBIMessage::resetHeader()
{
    if (m_parsedHeader) {
        char *method = m_parsedHeader->method;
        ogs_sbi_message_free(m_message);
        m_parsedHeader->method = nullptr;
        ogs_sbi_header_free(m_parsedHeader);
        m_parsedHeader->method = method;
        m_parsedHeader->service.name = nullptr;
        m_parsedHeader->api.version = nullptr;
        for (size_t i = 0; i < OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT && m_parsedHeader->resource.component[i]; i++) {
            m_parsedHeader->resource.component[i] = nullptr;
        }
        m_parsedHeader = nullptr;
    }
}

static void clear_header(ogs_sbi_header_t *header)
{

    if (header->uri) {
       char *method = ogs_strdup(header->method);
       ogs_sbi_header_free(header);
       header->method = nullptr;
       header->method = method;
       header->service.name = nullptr;
       header->api.version = nullptr;
       for (size_t i = 0; i < OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT && header->resource.component[i]; i++) {
            header->resource.component[i] = nullptr;
        }

    }

}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
