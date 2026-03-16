/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Open5GS SBI Object class
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

#include "ogs-app.h"
#include "ogs-sbi.h"

#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "common.hh"
#include "App.hh"
#include "TimerFunc.hh"
#include "Open5GSEvent.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSSBIStream.hh"

#include "Open5GSSBIObject.hh"

MBSF_NAMESPACE_START

Open5GSSBIObject::Open5GSSBIObject()
    :m_ogsObject(new ogs_sbi_object_t{})
{
     ogs_list_init(&m_ogsObject->xact_list);
}

Open5GSSBIObject::Open5GSSBIObject(ogs_sbi_object_t *object)
    :m_ogsObject(object)
{
     ogs_list_init(&m_ogsObject->xact_list);
}

Open5GSSBIObject::~Open5GSSBIObject()
{
    ogs_sbi_xact_remove_all(m_ogsObject);
    ogs_sbi_object_free(m_ogsObject);
    delete m_ogsObject;

}

ogs_sbi_xact_t *Open5GSSBIObject::discoverAndSend(ogs_pool_id_t stream_id, ogs_sbi_service_type_e service_type, ogs_sbi_discovery_option_t *discovery_option, ogs_sbi_build_f build, void *context, void *data)
{
    ogs_sbi_xact_t *xact = nullptr;
    int rv;
    ogs_pool_id_t xact_id = 0;

    if (!m_ogsObject) {
        m_ogsObject = new ogs_sbi_object_t{};
    }

    xact = ogs_sbi_xact_add(xact_id, m_ogsObject, service_type, discovery_option, build, (void *)context, (void *)data);

    if (!xact) {
        ogs_error("discoverAndSend() failed as adding transaction failed");
        return nullptr;
    }

    xact->assoc_stream_id = stream_id;

    rv = ogs_sbi_discover_and_send(xact);
    if (rv != OGS_OK) {
        ogs_error("discoverAndSend() failed");
        ogs_sbi_xact_remove(xact);
    }
    return xact;
}

ogs_sbi_xact_t *Open5GSSBIObject::discoverOnly(ogs_pool_id_t stream_id, ogs_sbi_service_type_e service_type, ogs_sbi_discovery_option_t *discovery_option)
{
    ogs_sbi_xact_t *xact;
    int rv;
    ogs_pool_id_t xact_id = 0;

    if (!m_ogsObject) {
        m_ogsObject = new ogs_sbi_object_t{};
    }

    xact = ogs_sbi_xact_add(xact_id, m_ogsObject, service_type, discovery_option, nullptr, nullptr, nullptr);
    if (!xact) {
        ogs_error("discoverOnly() failed as adding transaction failed");
        return nullptr;
    }
    xact->assoc_stream_id = stream_id;

    rv = ogs_sbi_discover_only(xact);
    if (rv != OGS_OK) {
        ogs_error("discoverOnly() failed");
        ogs_sbi_xact_remove(xact);
        return nullptr;
    }
    return xact;
}

void Open5GSSBIObject::setNFInstance(ogs_sbi_service_type_e service_type, ogs_sbi_nf_instance_t *nf_instance)
{
    OGS_SBI_SETUP_NF_INSTANCE(m_ogsObject->service_type_array[service_type], nf_instance);
}

void Open5GSSBIObject::addData(ogs_pool_id_t xact_id, Data data) {
     std::lock_guard<std::recursive_mutex> lock(m_mutex);
     m_xactTable[xact_id] = data;
}


const Open5GSSBIObject::Data& Open5GSSBIObject::getData(const ogs_pool_id_t xact_id) const
{
    return  m_xactTable.at(xact_id);
}

Open5GSSBIObject::Data& Open5GSSBIObject::getData(const ogs_pool_id_t xact_id)
{
    return  m_xactTable.at(xact_id);
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

