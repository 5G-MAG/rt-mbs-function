#ifndef _MBS_TF_OPEN5GS_YAML_DOCUMENT_HH_
#define _MBS_TF_OPEN5GS_YAML_DOCUMENT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS YAML document interface
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

#include <memory>

#include "common.hh"
#include "Open5GSYamlIter.hh"

MBSF_NAMESPACE_START

class Open5GSYamlDocument {
public:
    Open5GSYamlDocument(void *document) :m_document(document) {};
    Open5GSYamlDocument() = delete;
    Open5GSYamlDocument(Open5GSYamlDocument &&other) :m_document(other.m_document) {};
    Open5GSYamlDocument(const Open5GSYamlDocument &other) :m_document(other.m_document) {};
    Open5GSYamlDocument &operator=(Open5GSYamlDocument &&other) {m_document = other.m_document; return *this;};
    Open5GSYamlDocument &operator=(const Open5GSYamlDocument &other) {m_document = other.m_document; return *this;};
    virtual ~Open5GSYamlDocument() {};

    void *ogsDocument() const { return m_document; };

private:
    void *m_document;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_YAML_DOCUMENT_HH_ */
