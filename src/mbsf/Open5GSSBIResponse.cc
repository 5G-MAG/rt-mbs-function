/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Response interface
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author: David Waring <david.waring2@bbc.co.uk>
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

#include "common.hh"
#include "CaseInsensitiveTraits.hh"

#include "Open5GSSBIResponse.hh"

MBSF_NAMESPACE_START

bool Open5GSSBIResponse::headerSet(const std::string &field, const std::string &value)
{
    typedef std::string::value_type C;
    std::basic_string_view<C, CaseInsensitiveTraits<C> > lfield(field.c_str());
    // If header already exists, replace it.
    for (auto hi = ogs_hash_first(m_response->http.headers); hi; hi = ogs_hash_next(hi)) {
        const char *hdrfield;
        char *hdrval;
        int hdrfield_len;
        ogs_hash_this(hi, reinterpret_cast<const void**>(&hdrfield), &hdrfield_len, reinterpret_cast<void**>(&hdrval));

        std::basic_string_view<C, CaseInsensitiveTraits<C> > hdr_field(hdrfield);
        if (lfield.compare(hdr_field)==0) {
            if (value == hdrval) return false;
            ogs_hash_set(m_response->http.headers, reinterpret_cast<const void*>(hdrfield), hdrfield_len, reinterpret_cast<const void*>(ogs_strdup(value.c_str())));
            return true;
        }
    }

    // Add new header
    ogs_hash_set(m_response->http.headers, ogs_strdup(field.c_str()), OGS_HASH_KEY_STRING, ogs_strdup(value.c_str()));

    return true;
}

const char *Open5GSSBIResponse::getHeader(const char *header)
{
    ogs_hash_index_t *hi;
    for (hi = ogs_hash_first(this->ogsSBIResponse()->http.headers);
            hi; hi = ogs_hash_next(hi)) {
        if (!ogs_strcasecmp((const char*)ogs_hash_this_key(hi), header)) {
            return (const char*)ogs_hash_this_val(hi);
        } else {
            ogs_debug( "Header not present %s", header);
            return NULL;
        }
    }
    return NULL;
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
