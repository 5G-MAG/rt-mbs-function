#ifndef _MBSF_OBJ_REPAIR_PARAMETERS_HH_
#define _MBSF_OBJ_REPAIR_PARAMETERS_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Obj Repair Parameters class
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


#include "openapi/model/ObjectRepairParameters.h"
#include "common.hh"

MBSF_NAMESPACE_START

class ObjRepairParameters {
public:

    ObjRepairParameters(fiveg_mag_reftools::CJson &json, bool as_request);
    ObjRepairParameters(const std::shared_ptr<reftools::mbsf::ObjectRepairParameters> &object_repair_parameters);
    ObjRepairParameters(const std::optional<std::string > &object_distribution_base_locator, const std::optional<std::string > &object_repair_base_locator);
    ObjRepairParameters(const std::string &user_data_ing_session_id, const std::string &distribution_session_info_key, const std::optional<std::string > &object_distribution_base_locator = std::nullopt);
    ObjRepairParameters() = delete;
    ObjRepairParameters(ObjRepairParameters &&other) = delete;
    ObjRepairParameters(const ObjRepairParameters &other) = delete;
    ObjRepairParameters &operator=(ObjRepairParameters &&other) = delete;
    ObjRepairParameters &operator=(const ObjRepairParameters &other) = delete;

    virtual ~ObjRepairParameters();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::ObjectRepairParameters> &objectRepairParameters() const {return m_objectRepairParameters;};
    const std::optional<std::string > &objectDistributionBaseLocator() const {return m_objectRepairParameters->getObjectDistributionBaseLocator();};
    const std::optional<std::string > &objectRepairBaseLocator() const {return m_objectRepairParameters->getObjectRepairBaseLocator();};

private:

    std::shared_ptr<reftools::mbsf::ObjectRepairParameters> m_objectRepairParameters;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OBJ_REPAIR_PARAMETERS_HH_ */
