/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Availability Object Repair Parameters class
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

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// standard template library includes
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <iostream>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "openapi/model/ObjectRepairParameters.h"
#include "openapi/model/BackOffParameters.h"

// Header include for this class
#include "ObjRepairParameters.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::ObjectRepairParameters;
using reftools::mbsf::BackOffParameters;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

static std::string remove_trailing_slash(std::string input);

ObjRepairParameters::ObjRepairParameters(CJson &json, bool as_request)
    :m_objectRepairParameters(new ObjectRepairParameters(json, as_request))
{
}

ObjRepairParameters::ObjRepairParameters(const std::shared_ptr<reftools::mbsf::ObjectRepairParameters> &object_repair_parameters)
    :m_objectRepairParameters(object_repair_parameters)
{
}

ObjRepairParameters::ObjRepairParameters(const std::optional<std::string > &object_distribution_base_locator, const std::optional<std::string > &object_repair_base_locator)
    :m_objectRepairParameters(new ObjectRepairParameters())
{
    m_objectRepairParameters->setObjectDistributionBaseLocator(object_distribution_base_locator);
    m_objectRepairParameters->setObjectRepairBaseLocator(object_repair_base_locator);

}

ObjRepairParameters::ObjRepairParameters(const std::string &user_data_ing_session_id, const std::string &distribution_session_info_key, const std::optional<std::string > &object_distribution_base_locator)
    :m_objectRepairParameters(new ObjectRepairParameters())
{
    if(App::self().context()->objectRepairParameters.objectRepairBaseLocator.has_value() && 
		    App::self().context()->objectRepairParameters.backOffParametersOffsetTime && 
		    App::self().context()->objectRepairParameters.backOffParametersRandomTimePeriod)
    {
        if(App::self().context()->objectRepairParameters.objectRepairBaseLocator.has_value()) {
	    std::string obj_repair_base_locator = remove_trailing_slash(App::self().context()->objectRepairParameters.objectRepairBaseLocator.value()) + "/" + user_data_ing_session_id + "/"+ distribution_session_info_key;
        
            m_objectRepairParameters->setObjectRepairBaseLocator(obj_repair_base_locator);
	}
	m_objectRepairParameters->setObjectDistributionBaseLocator(object_distribution_base_locator);
         
        std::shared_ptr< reftools::mbsf::BackOffParameters > back_off_params(new BackOffParameters());
        back_off_params->setOffsetTime(App::self().context()->objectRepairParameters.backOffParametersOffsetTime);
        back_off_params->setRandomTimePeriod(App::self().context()->objectRepairParameters.backOffParametersRandomTimePeriod);
        
	m_objectRepairParameters->setBackOffParameters(back_off_params);
    }
}

ObjRepairParameters::~ObjRepairParameters()
{
}


CJson ObjRepairParameters::json(bool as_request = false) const
{
    return m_objectRepairParameters->toJSON(as_request);
}

static std::string remove_trailing_slash(std::string input) {
    if (!input.empty() && input.back() == '/') {
        return input.substr(0, input.size() - 1);
    }
    return input;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

