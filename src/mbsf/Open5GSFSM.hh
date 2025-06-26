#ifndef _MBS_TF_OPEN5GS_FSM_HH_
#define _MBS_TF_OPEN5GS_FSM_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS FSM interface
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

MBSF_NAMESPACE_START

class Open5GSFSM {
public:
    Open5GSFSM(ogs_fsm_t *fsm) :m_ogsFsm(fsm) {};
    Open5GSFSM() = delete;
    Open5GSFSM(Open5GSFSM &&other) = delete;
    Open5GSFSM(const Open5GSFSM &other) = delete;
    Open5GSFSM &operator=(Open5GSFSM &&other) = delete;
    Open5GSFSM &operator=(const Open5GSFSM &other) = delete;
    virtual ~Open5GSFSM() {};

private:
    ogs_fsm_t *m_ogsFsm;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_FSM_HH_ */
