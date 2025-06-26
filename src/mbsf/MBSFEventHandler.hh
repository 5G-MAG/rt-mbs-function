#ifndef _MBS_TF_MBSF_EVENT_HANDLER_HH_
#define _MBS_TF_MBSF_EVENT_HANDLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSF Open5GS Event Handler
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

#include "common.hh"
#include "EventHandler.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"

#define mbsf_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, ogs_event_get_name(__pe))

MBSF_NAMESPACE_START

class MBSFEventHandler : public EventHandler {
public:
    MBSFEventHandler() :EventHandler() {};
    MBSFEventHandler(MBSFEventHandler &&other) = delete;
    MBSFEventHandler(const MBSFEventHandler &other) = delete;
    MBSFEventHandler &operator=(MBSFEventHandler &&other) = delete;
    MBSFEventHandler &operator=(const MBSFEventHandler &other) = delete;
    virtual ~MBSFEventHandler() = default;

    virtual void dispatch(Open5GSFSM &fsm, Open5GSEvent &event);

private:
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSF_EVENT_HANDLER_HH_ */
