#ifndef _MBSF_EXTERNAL_SERVICE_AREA_HH_
#define _MBSF_EXTERNAL_SERVICE_AREA_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Service Area class
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "mb-smf-service-consumer.h"

#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/ExternalMbsServiceArea.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class ExternalServiceArea {
public:

    ExternalServiceArea(fiveg_mag_reftools::CJson &json, bool as_request);
    ExternalServiceArea(const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &external_mbs_service_area);
    ExternalServiceArea() = delete;
    ExternalServiceArea(ExternalServiceArea &&other) = delete;
    ExternalServiceArea(const ExternalServiceArea &other) = delete;
    ExternalServiceArea &operator=(ExternalServiceArea &&other) = delete;
    ExternalServiceArea &operator=(const ExternalServiceArea &other) = delete;

    virtual ~ExternalServiceArea();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &getExternalMbsServiceArea() const {return m_externalMbsServiceArea;};
    const reftools::mbsf::ExternalMbsServiceArea::GeographicAreaListType &getGeographicAreaList() const {return m_externalMbsServiceArea->getGeographicAreaList();};
    const reftools::mbsf::ExternalMbsServiceArea::CivicAddressListType &getCivicAddressList() const {return m_externalMbsServiceArea->getCivicAddressList();};

    mb_smf_sc_ext_mbs_service_area_t *populateExternalServiceArea();
    void geographicAreas(mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area);
    void civicAddresses(mb_smf_sc_ext_mbs_service_area_t *ext_mbs_service_area);

private:
    std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> m_externalMbsServiceArea;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_EXTERNAL_SERVICE_AREA_HH_ */
