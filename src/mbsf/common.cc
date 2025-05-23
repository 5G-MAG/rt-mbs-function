/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Common values and macros
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
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

#include "ogs-core.h"

int __mbsf_log_domain;

void initialise_logging(void)
{
    ogs_log_install_domain(&__mbsf_log_domain, "MBSF", ogs_core()->log.level);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
