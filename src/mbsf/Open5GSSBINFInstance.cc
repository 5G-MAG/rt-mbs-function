/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: Open5GS SBI NF Instance class
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

#include "Open5GSSBINFInstance.hh"

MBSF_NAMESPACE_START


Open5GSSBINFInstance::Open5GSSBINFInstance(ogs_sbi_nf_instance_t *instance, bool owner)
    :m_ogsSbiNfInstance(instance)
    ,m_owner(owner)
{
    if (!m_ogsSbiNfInstance) {
            throw std::runtime_error("Open5GSSBINFInstance failed");
        }

}

Open5GSSBINFInstance::Open5GSSBINFInstance(const std::string &id, bool owner)
    :m_ogsSbiNfInstance()
    ,m_owner(owner)
{
    std::string nf_id = id;
    m_ogsSbiNfInstance = ogs_sbi_nf_instance_find(nf_id.data());
    if (!m_ogsSbiNfInstance) {
            throw std::runtime_error("Open5GSSBINFInstance failed");
        }

}

Open5GSSBINFInstance::~Open5GSSBINFInstance()
{
    if(m_owner) ogs_sbi_nf_instance_remove(m_ogsSbiNfInstance);

}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

