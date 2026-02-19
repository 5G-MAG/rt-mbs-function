/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Open5GS SBI Request interface
 ******************************************************************************
 * Copyright: (C)2024-2025 British Broadcasting Corporation
 * Author: David Waring <david.waring2@bbc.co.uk>
 *         Dev Audsin <dev.audsin@bbc.co.uk>
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

#include <cctype>
#include <string>
#include <sstream>
#include <string_view>

#include "common.hh"
#include "CaseInsensitiveTraits.hh"
#include "Open5GSSBIMessage.hh"

#include "Open5GSSBIRequest.hh"

MBSF_NAMESPACE_START

Open5GSSBIRequest::Open5GSSBIRequest(const std::string &method, const std::string &uri, const std::string &apiVersion, const std::optional<std::string> &data, const std::optional<std::string> &type)
    :m_request(ogs_sbi_request_new())
    ,m_owner(true)
{

    //m_request = ogs_sbi_request_new();
    m_request->h.method = ogs_strdup((char *)method.c_str());
    m_request->h.uri = ogs_strdup((char *)uri.c_str());
    m_request->h.api.version = ogs_strdup((char *)apiVersion.c_str());

    if (data) {
        m_request->http.content = ogs_strdup(const_cast<char*>(data->c_str()));
        m_request->http.content_length = data->size();
    } else {
        m_request->http.content = nullptr;
        m_request->http.content_length = 0;
    }

    if (type) {
        ogs_sbi_header_set(m_request->http.headers, "Content-Type", type->c_str());
    } else {
        ogs_sbi_header_set(m_request->http.headers, "Content-Type", nullptr);
    }

}

Open5GSSBIRequest::Open5GSSBIRequest(Open5GSSBIMessage &message)
    :m_request(ogs_sbi_build_request(message.ogsSBIMessage()))
    ,m_owner(true)

{

}

std::string Open5GSSBIRequest::headerValue(const std::string &field, const std::string &defval) const
{
    typedef std::string::value_type C;
    std::basic_string_view<C, CaseInsensitiveTraits<C> > lfield(field.c_str());
    for (auto hi = ogs_hash_first(m_request->http.headers); hi; hi = ogs_hash_next(hi)) {
        std::basic_string_view<C, CaseInsensitiveTraits<C> > hdr_field(reinterpret_cast<const char*>(ogs_hash_this_key(hi)));
        if (lfield.compare(hdr_field)==0) {
            return std::string(reinterpret_cast<const char*>(ogs_hash_this_val(hi)));
        }
    }

    return defval;
}

const char *Open5GSSBIRequest::resourceComponent(size_t idx) const
{
    if (!m_request) return nullptr;
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) return nullptr;
    return m_request->h.resource.component[idx];
}

std::string Open5GSSBIRequest::parameterValue(const std::string &field, const std::string &defval) const
{
    for (auto hi = ogs_hash_first(m_request->http.params); hi; hi = ogs_hash_next(hi)) {
        std::string_view hdr_field(reinterpret_cast<const char*>(ogs_hash_this_key(hi)));
        if (field.compare(hdr_field)==0) {
            return std::string(reinterpret_cast<const char*>(ogs_hash_this_val(hi)));
        }
    }

    return defval;
}

void Open5GSSBIRequest::headersMap(const Open5GSSBIRequest::HeadersMap &map) const
{
    for (auto hi = ogs_hash_first(m_request->http.headers); hi; hi = ogs_hash_next(hi)) {
        CaseInsensitiveString hdr_field(reinterpret_cast<const char*>(ogs_hash_this_key(hi)));
        auto it = map.find(hdr_field);
        if (it != map.end()) {
            it->second(it->first, reinterpret_cast<const char*>(ogs_hash_this_val(hi)));
        }
    }
}

void Open5GSSBIRequest::parametersMap(const Open5GSSBIRequest::ParametersMap &map) const
{
    for (auto hi = ogs_hash_first(m_request->http.params); hi; hi = ogs_hash_next(hi)) {
        std::string param_field(reinterpret_cast<const char*>(ogs_hash_this_key(hi)));
        auto it = map.find(param_field);
        if (it != map.end()) {
            it->second(it->first, reinterpret_cast<const char*>(ogs_hash_this_val(hi)));
        }
    }
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
