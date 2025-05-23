#ifndef _MBSF_COMMON_HH_
#define _MBSF_COMMON_HH_
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

#define MBSF_NAMESPACE com::fiveg_mag::ref_tools::mbsf
#define MBSF_NAMESPACE_START namespace MBSF_NAMESPACE {
#define MBSF_NAMESPACE_STOP  }
#define MBSF_NAMESPACE_USING using namespace MBSF_NAMESPACE
#define MBSF_NAMESPACE_NAME(a) MBSF_NAMESPACE::a

extern int __mbsf_log_domain;
#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __mbsf_log_domain

extern void initialise_logging(void);

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_COMMON_HH_ */
