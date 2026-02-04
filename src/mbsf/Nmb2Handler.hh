#ifndef _MBSF_NMB2_HANDLER_HH_
#define _MBSF_NMB2_HANDLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Nmb2 DistSession handler
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

#include "common.hh"

MBSF_NAMESPACE_START

class Open5GSEvent;

class Nmb2Handler {
public:
    Nmb2Handler();
    Nmb2Handler(Nmb2Handler &&other) = delete;
    Nmb2Handler(const Nmb2Handler &other) = delete;
    Nmb2Handler &operator=(Nmb2Handler &&other) = delete;
    Nmb2Handler &operator=(const Nmb2Handler &other) = delete;

    static bool processEvent(Open5GSEvent &event);

private:

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_NMB2_HANDLER_HH_ */
