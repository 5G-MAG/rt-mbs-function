#ifndef _MBSF_OPEN5GS_SBI_HEADER_HH_
#define _MBSF_OPEN5GS_SBI_HEADER_HH_
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

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSBIHeader {
public:
    Open5GSSBIHeader(ogs_sbi_header_t *hdr, bool owner=false) :m_header(hdr),m_owner(owner) {};
    Open5GSSBIHeader() :m_header(nullptr),m_owner(false) {};
    Open5GSSBIHeader(Open5GSSBIHeader &&other) :m_header(other.m_header),m_owner(other.m_owner) {other.m_owner = false;};
    Open5GSSBIHeader(const Open5GSSBIHeader &other) :m_header(other.m_header),m_owner(false) {};
    Open5GSSBIHeader &operator=(Open5GSSBIHeader &&other) {m_header = other.m_header; m_owner = other.m_owner; other.m_owner = false; return *this;};
    Open5GSSBIHeader &operator=(const Open5GSSBIHeader &other) {m_header = other.m_header; m_owner = false; return *this;};

    virtual ~Open5GSSBIHeader();

    ogs_sbi_header_t *ogsSBIHeader() { return m_header; };
    const ogs_sbi_header_t *ogsSBIHeader() const { return m_header; };
    void setOwner(bool owner) { m_owner = owner; };

    const char *serviceName() const { return m_header?(m_header->service.name):nullptr; };
    const char *apiVersion() const { return m_header?(m_header->api.version):nullptr; };
    const char *resourceComponent(size_t idx) const;
    const char *method() const { return m_header?(m_header->method):nullptr; };
    const char *uri() const { return m_header?(m_header->uri):nullptr; };

    Open5GSSBIHeader &serviceName(const char *service_name);
    Open5GSSBIHeader &apiVersion(const char *api_version);
    Open5GSSBIHeader &resourceComponent(size_t idx, const char *resource_component);
    Open5GSSBIHeader &method(const char *method);
    Open5GSSBIHeader &uri(const char *uri);

    operator bool() const { return !!m_header; };

private:
    void ensureHeader();

    ogs_sbi_header_t *m_header;
    bool m_owner;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OPEN5GS_SBI_HEADER_HH_ */
