/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS YAML iterator
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

#include "common.hh"
#include "Open5GSYamlDocument.hh"

#include "Open5GSYamlIter.hh"

MBSF_NAMESPACE_START

Open5GSYamlIter::Open5GSYamlIter(const Open5GSYamlDocument &document)
{
    ogs_yaml_iter_init(&m_iterator, reinterpret_cast<yaml_document_t*>(document.ogsDocument()));
}

Open5GSYamlIter::Open5GSYamlIter(Open5GSYamlIter &parent)
{
    ogs_yaml_iter_recurse(&parent.m_iterator, &m_iterator);
}

int Open5GSYamlIter::next()
{
    return ogs_yaml_iter_next(&m_iterator);
}

int Open5GSYamlIter::type() const
{
    return ogs_yaml_iter_type(const_cast<ogs_yaml_iter_t*>(&m_iterator));
}

const char *Open5GSYamlIter::key() const
{
    return ogs_yaml_iter_key(const_cast<ogs_yaml_iter_t*>(&m_iterator));
}

const char *Open5GSYamlIter::value() const
{
    return ogs_yaml_iter_value(const_cast<ogs_yaml_iter_t*>(&m_iterator));
}

bool Open5GSYamlIter::valueBool() const
{
    return ogs_yaml_iter_bool(const_cast<ogs_yaml_iter_t*>(&m_iterator));
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
