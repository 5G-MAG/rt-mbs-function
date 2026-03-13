#ifndef _MBSF_APPLICATION_SERVICE_DESC_HH_
#define _MBSF_APPLICATION_SERVICE_DESC_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Availability Information class
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

#include "mb-smf-service-consumer.h"

#include <memory>

#include "openapi/model/ApplicationServiceDescription.h"
#include "common.hh"

MBSF_NAMESPACE_START

class ApplicationServiceDesc {
public:

    ApplicationServiceDesc(fiveg_mag_reftools::CJson &json, bool as_request);
    ApplicationServiceDesc(const std::shared_ptr<reftools::mbsf::ApplicationServiceDescription> &application_service_description);
    ApplicationServiceDesc(const std::string &entry_point_locator, const std::string &content_type);
    ApplicationServiceDesc(const std::string &entry_point_locator);
    ApplicationServiceDesc(const std::string &obj_distr_uri, const std::string &obj_ing_uri, const std::string &obj_acq_id);
    ApplicationServiceDesc() = delete;
    ApplicationServiceDesc(ApplicationServiceDesc &&other) = delete;
    ApplicationServiceDesc(const ApplicationServiceDesc &other) = delete;
    ApplicationServiceDesc &operator=(ApplicationServiceDesc &&other) = delete;
    ApplicationServiceDesc &operator=(const ApplicationServiceDesc &other) = delete;

    virtual ~ApplicationServiceDesc() {
    }

    void obtainContentType();
    ApplicationServiceDesc &populateContentTypes();
    std::string extractContentTypeFromExtension(const std::string &acq_id);

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::ApplicationServiceDescription> &applicationServiceDescription() const {return m_applicationServiceDescription;};
    const std::string &entryPointLocator() const {return m_applicationServiceDescription->getEntryPointLocator();};
    const std::string &contentType() const {return m_applicationServiceDescription->getContentType();};

private:

    std::shared_ptr<reftools::mbsf::ApplicationServiceDescription> m_applicationServiceDescription;
    std::unordered_map<std::string, std::string> m_contentTypes;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_APPLICATION_SERVICE_DESC_HH */
