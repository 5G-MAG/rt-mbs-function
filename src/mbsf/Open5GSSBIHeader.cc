/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Open5GS SBI Header interface
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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
#include "ogs-proto.h"

#include <stdexcept>
#include <memory>
#include <cstring>

#include "common.hh"

#include "Open5GSSBIHeader.hh"

MBSF_NAMESPACE_START

Open5GSSBIHeader::~Open5GSSBIHeader()
{
    //ogs_debug("~Open5GSSBIHeader: message=%p, owner=%s", m_header, m_owner?"true":"false");
    if (m_header && m_owner) {
        ogs_sbi_header_free(m_header);
        delete m_header;
        m_header = nullptr;
    }
}

const char *Open5GSSBIHeader::resourceComponent(size_t idx) const
{
    if (!m_header) return nullptr;
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) return nullptr;
    return m_header->resource.component[idx];
}

Open5GSSBIHeader &Open5GSSBIHeader::resourceComponent(size_t idx, const char *resource_component)
{
    ensureHeader();
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) throw std::runtime_error("Maximum number of resource component failed");
    if (m_header->resource.component[idx]) ogs_free(m_header->resource.component[idx]);
    m_header->resource.component[idx] = ogs_strdup(resource_component);
    return *this;
}

Open5GSSBIHeader &Open5GSSBIHeader::method(const char *method)
{
    ensureHeader();
    if (m_header->method) ogs_free(m_header->method);
    m_header->method = ogs_strdup(method);
    return *this;
};

Open5GSSBIHeader &Open5GSSBIHeader::serviceName(const char *service_name)
{
    ensureHeader();
    if (m_header->service.name) ogs_free(m_header->service.name);
    m_header->service.name = ogs_strdup(service_name);
    return *this;
}

Open5GSSBIHeader &Open5GSSBIHeader::apiVersion(const char *api_version)
{
    ensureHeader();
    if (m_header->api.version) ogs_free(m_header->api.version);
    m_header->api.version = ogs_strdup(api_version);
    return *this;
}

Open5GSSBIHeader &Open5GSSBIHeader::uri(const char *uri)
{
    ensureHeader();
    if (m_header->uri) ogs_free(m_header->uri);
    m_header->uri = ogs_strdup(uri);
    return *this;
}

/*** private: ***/

void Open5GSSBIHeader::ensureHeader()
{
    if (!m_header) {
        m_header = new ogs_sbi_header_t{};
        m_owner = true;
    }
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
