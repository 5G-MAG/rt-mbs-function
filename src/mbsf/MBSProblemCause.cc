/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Problem Cause
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
#include "ogs-sbi.h"

#include "common.hh"
#include "App.hh"

#include "MBSProblemCause.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ProblemCause;

MBSF_NAMESPACE_START

const std::unordered_map<std::string, const ProblemCause *> MBSProblemCause::propagationTable = {
        // 400 Bad Request equivalents
        { "INVALID_MBS_SERVICE_INFO",             &MBSProblemCause::INVALID_MBS_SERVICE_INFO },
        { "FILTER_RESTRICTIONS_NOT_RESPECTED",    &ProblemCause::UNSPECIFIED_MSG_FAILURE },
        { "ERROR_INPUT_PARAMETERS",               &ProblemCause::UNSPECIFIED_MSG_FAILURE },

        // 403 Forbidden equivalents
        { "MBS_SERVICE_INFO_NOT_AUTHORIZED",      &MBSProblemCause::MBS_SERVICE_INFO_NOT_AUTHORIZED },
        { "MBS_POLICY_CONTEXT_DENIED",            nullptr },
        { "MBS_SESSION_ALREADY_CREATED",          &MBSProblemCause::MBS_DIST_SESSION_ALREADY_CREATED },
        { "OVERLAPPING_MBS_SERVICE_AREA",         &MBSProblemCause::OVERLAPPING_MBS_SERVICE_AREA },

        // 404 Not Found equivalents
        { "UNKNOWN_TMGI",                         nullptr },
        { "UNKNOWN_MBS_SESSION",                  nullptr },
        { "UNKNOWN_MBS_SERVICE_AREA",             &MBSProblemCause::UNKNOWN_MBS_SERVICE_AREA }
    };


const fiveg_mag_reftools::ProblemCause MBSProblemCause::INVALID_MBS_SERVICE_INFO(fiveg_mag_reftools::ProblemCause::registerCause("INVALID_MBS_SERVICE_INFO", OGS_SBI_HTTP_STATUS_BAD_REQUEST, "Bad Request", "The provided MBS Service Information is invalid (e.g. invalid QoS reference), incorrect or insufficient to perform MBS policy authorization."));

const fiveg_mag_reftools::ProblemCause MBSProblemCause::MBS_SERVICE_AREA_NOT_SUPPORTED(fiveg_mag_reftools::ProblemCause::registerCause("MBS_SERVICE_AREA_NOT_SUPPORTED", OGS_SBI_HTTP_STATUS_FORBIDDEN, "Forbidden", "The requested MBS Service Area is not supported by the 3GPP Core Network."));

const fiveg_mag_reftools::ProblemCause MBSProblemCause::MBS_SERVICE_INFO_NOT_AUTHORIZED(fiveg_mag_reftools::ProblemCause::registerCause("MBS_SERVICE_INFO_NOT_AUTHORIZED", OGS_SBI_HTTP_STATUS_FORBIDDEN, "Forbidden", "The provided MBS Service Information is rejected."));

const fiveg_mag_reftools::ProblemCause MBSProblemCause::MBS_DIST_SESSION_ALREADY_CREATED(fiveg_mag_reftools::ProblemCause::registerCause("MBS_DIST_SESSION_ALREADY_CREATED", OGS_SBI_HTTP_STATUS_FORBIDDEN, "Duplicate MBS Session ID", "The requested MBS Distribution Session has already been created"));

const fiveg_mag_reftools::ProblemCause MBSProblemCause::OVERLAPPING_MBS_SERVICE_AREA(fiveg_mag_reftools::ProblemCause::registerCause("OVERLAPPING_MBS_SERVICE_AREA", OGS_SBI_HTTP_STATUS_FORBIDDEN, "Forbidden", "The provided MBS service area overlaps with the MBS service area of an existing MBS Distribution Session that shares the same MBS session Identifier"));

const fiveg_mag_reftools::ProblemCause MBSProblemCause::UNKNOWN_MBS_SERVICE_AREA(fiveg_mag_reftools::ProblemCause::registerCause("UNKNOWN_MBS_SERVICE_AREA", OGS_SBI_HTTP_STATUS_NOT_FOUND , "Not Found", "that the requested MBS service area (e.g., identified by the Area Session ID) cannot be found"));


std::optional<ProblemCause> MBSProblemCause::lookup(const std::string& mbsmf_problem_cause) {
  auto it = propagationTable.find(mbsmf_problem_cause);
  if (it == propagationTable.end()) return std::nullopt;
  return (it->second)?std::optional<ProblemCause>(*it->second):std::optional<ProblemCause>();
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
