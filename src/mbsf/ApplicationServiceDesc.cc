/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Application Service Description class
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
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string_view>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "openapi/model/ApplicationServiceDescription.h"

// Header include for this class
#include "ApplicationServiceDesc.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::ApplicationServiceDescription;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

static std::string remove_trailing_slash(std::string input);

ApplicationServiceDesc::ApplicationServiceDesc(CJson &json, bool as_request)
    :m_applicationServiceDescription(new ApplicationServiceDescription(json, as_request))
    ,m_contentTypes()
{
}

ApplicationServiceDesc::ApplicationServiceDesc(const std::shared_ptr<reftools::mbsf::ApplicationServiceDescription> &application_service_description)
    :m_applicationServiceDescription(application_service_description)
    ,m_contentTypes()
{
}

ApplicationServiceDesc::ApplicationServiceDesc(const std::string &entry_point_locator, const std::string &content_type)
    :m_applicationServiceDescription(new ApplicationServiceDescription())
    ,m_contentTypes()	
{
    m_applicationServiceDescription->setEntryPointLocator(entry_point_locator);
    m_applicationServiceDescription->setContentType(content_type);
}

ApplicationServiceDesc::ApplicationServiceDesc(const std::string &obj_distr_uri, const std::string &obj_ing_uri, const std::string &obj_acq_id)
    :m_applicationServiceDescription(new ApplicationServiceDescription())
    ,m_contentTypes()	
{
    //std::string entry_point_locator = remove_trailing_slash(obj_distr_uri) + "/" + obj_acq_id;
    //url = remove_trailing_slash(obj_ing_uri) + "/" + obj_acq_id;
    std::string entry_point_locator = remove_trailing_slash(obj_ing_uri) + "/" + obj_acq_id;

    if(m_contentTypes.empty()) {
        populateContentTypes();
    }

    const std::string &content_type = extractContentTypeFromExtension(obj_acq_id);

    m_applicationServiceDescription->setEntryPointLocator(entry_point_locator);
    m_applicationServiceDescription->setContentType(content_type);

}

CJson ApplicationServiceDesc::json(bool as_request = false) const
{
    return m_applicationServiceDescription->toJSON(as_request);
}

//Temporary method to populate Content type from extension in Linux.
//This method will be removed when the contentType comes from the Application Provider.
ApplicationServiceDesc &ApplicationServiceDesc::populateContentTypes() 
{
    std::ifstream file("/etc/mime.types");
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string content_type;
        iss >> content_type;

        std::string ext;
        while (iss >> ext) {
            m_contentTypes[ext] = content_type;
        }
    }
    return *this;
}

std::string ApplicationServiceDesc::extractContentTypeFromExtension(const std::string& acq_id) {

    auto pos = acq_id.find_last_of('.');
    if (pos == std::string::npos) return "application/octet-stream";

    std::string ext = acq_id.substr(pos + 1);
    auto it = m_contentTypes.find(ext);
    return it != m_contentTypes.end() ? it->second : "application/octet-stream";
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

