#ifndef _MBSF_LOCAL_EVENTS_HH_
#define _MBSF_LOCAL_EVENTS_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Local Events
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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

#include "common.hh"
#include "Open5GSEvent.hh"

MBSF_NAMESPACE_START

class LocalEvents {
public:
    typedef enum {
        SEND_NOTIFICATION = OGS_MAX_NUM_OF_PROTO_EVENT+1000,
        SEND_USER_DATA_ING_SESS_NOTIFICATION,
    } LocalEventIds;

    static const char *getEventName(Open5GSEvent &event) {
        if (event.id() < OGS_MAX_NUM_OF_PROTO_EVENT) return ogs_event_get_name(event.ogsEvent());
        if (event.id() == SEND_NOTIFICATION) return "SEND_NOTIFICATION";
        if (event.id() == SEND_USER_DATA_ING_SESS_NOTIFICATION) return "SEND_USER_DATA_ING_SESS_NOTIFICATION";
        return "Unknown event";
    };
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_LOCAL_EVENTS_HH_ */
