#ifndef _MBS_TF_OPEN5GS_YAML_ITER_HH_
#define _MBS_TF_OPEN5GS_YAML_ITER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS YAML Iterator class
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

MBSF_NAMESPACE_START

class Open5GSYamlDocument;

class Open5GSYamlIter {
public:
    Open5GSYamlIter(const Open5GSYamlDocument &document);
    Open5GSYamlIter(Open5GSYamlIter &parent);
    Open5GSYamlIter() = delete;
    Open5GSYamlIter(Open5GSYamlIter &&other) = delete;
    Open5GSYamlIter(const Open5GSYamlIter &other) = delete;
    Open5GSYamlIter &operator=(Open5GSYamlIter &&other) = delete;
    Open5GSYamlIter &operator=(const Open5GSYamlIter &other) = delete;
    virtual ~Open5GSYamlIter() {};

    int next();

    int type() const;
    const char *key() const;
    const char *value() const;
    bool valueBool() const;

private:
    ogs_yaml_iter_t m_iterator;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_YAML_ITER_HH_ */
