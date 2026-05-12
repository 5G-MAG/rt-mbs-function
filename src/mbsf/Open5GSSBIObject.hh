#ifndef _MBSF_OPEN5GS_SBI_OBJECT_HH_
#define _MBSF_OPEN5GS_SBI_OBJECT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Open5GS SBI Object interface
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

#include <memory>
#include <optional>
#include <any>
#include <mutex>
#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSSBIStream;
class Open5GSSBIMessage;

class Open5GSSBIObject {
public:

    using Data = std::any;

    Open5GSSBIObject();
    Open5GSSBIObject(ogs_sbi_object_t *object);
    Open5GSSBIObject(Open5GSSBIObject &&other) = delete;
    Open5GSSBIObject(const Open5GSSBIObject &other) = delete;
    Open5GSSBIObject &operator=(Open5GSSBIObject &&other) = delete;
    Open5GSSBIObject &operator=(const Open5GSSBIObject &other) = delete;
    virtual ~Open5GSSBIObject();

    ogs_sbi_xact_t *discoverAndSend(ogs_pool_id_t stream_id, ogs_sbi_service_type_e service_type, ogs_sbi_discovery_option_t *discovery_option, ogs_sbi_build_f build, void *context, void *data);
    ogs_sbi_xact_t *discoverOnly(ogs_pool_id_t stream_id, ogs_sbi_service_type_e service_type, ogs_sbi_discovery_option_t *discovery_option);
    ogs_sbi_object_t *ogsSBIObject() { return m_ogsObject; };

    void setNFInstance(ogs_sbi_service_type_e service_type, ogs_sbi_nf_instance_t *nf_instance);
    ogs_sbi_nf_instance_t *getNFInstance(ogs_sbi_service_type_e service_type);

    void addData(const ogs_pool_id_t xact_id, Data data);
    const Data& getData(const ogs_pool_id_t xact_id) const;
    Data& getData(const ogs_pool_id_t xact_id);


    operator bool() const { return !!m_ogsObject; };

private:
    ogs_sbi_object_t *m_ogsObject;
    mutable std::recursive_mutex m_mutex;
    std::map<ogs_pool_id_t, std::any> m_xactTable;

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OPEN5GS_SBI_OBJECT_HH_ */

