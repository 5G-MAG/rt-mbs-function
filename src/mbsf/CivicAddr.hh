#ifndef _MBSF_CIVIC_ADDRESS_HH_
#define _MBSF_CIVIC_ADDRESS_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS CivicAddr class
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
#include "openapi/model/CivicAddress.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class CivicAddr {
public:

    CivicAddr(fiveg_mag_reftools::CJson &json, bool as_request);
    CivicAddr(const std::shared_ptr<reftools::mbsf::CivicAddress> &civic_address);
    CivicAddr() = delete;
    CivicAddr(CivicAddr &&other) = delete;
    CivicAddr(const CivicAddr &other) = delete;
    CivicAddr &operator=(CivicAddr &&other) = delete;
    CivicAddr &operator=(const CivicAddr &other) = delete;

    virtual ~CivicAddr();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::CivicAddress> &getCivicAddress() const {return m_civicAddress;};
    const std::optional<std::string> &getCountry() const {return m_civicAddress->getCountry();};
    const std::optional<std::string> &getA1() const {return m_civicAddress->getA1();};
    const std::optional<std::string> &getA2() const {return m_civicAddress->getA2();};
    const std::optional<std::string> &getA3() const {return m_civicAddress->getA3();};
    const std::optional<std::string> &getA4() const {return m_civicAddress->getA4();};
    const std::optional<std::string> &getA5() const {return m_civicAddress->getA5();};
    const std::optional<std::string> &getA6() const {return m_civicAddress->getA6();};
    const std::optional<std::string> &getPRD() const {return m_civicAddress->getPRD();};
    const std::optional<std::string> &getPOD() const {return m_civicAddress->getPOD();};
    const std::optional<std::string> &getSTS() const {return m_civicAddress->getSTS();};
    const std::optional<std::string> &getHNO() const {return m_civicAddress->getHNO();};
    const std::optional<std::string> &getHNS() const {return m_civicAddress->getHNS();};
    const std::optional<std::string> &getLMK() const {return m_civicAddress->getLMK();};
    const std::optional<std::string> &getLOC() const {return m_civicAddress->getLOC();};
    const std::optional<std::string> &getNAM() const {return m_civicAddress->getNAM();};
    const std::optional<std::string> &getPC() const {return m_civicAddress->getPC();};
    const std::optional<std::string> &getBLD() const {return m_civicAddress->getBLD();};
    const std::optional<std::string> &getUNIT() const {return m_civicAddress->getUNIT();};
    const std::optional<std::string> &getFLR() const {return m_civicAddress->getFLR();};
    const std::optional<std::string> &getROOM() const {return m_civicAddress->getROOM();};
    const std::optional<std::string> &getPLC() const {return m_civicAddress->getPLC();};
    const std::optional<std::string> &getPCN() const {return m_civicAddress->getPCN();};
    const std::optional<std::string> &getPOBOX() const {return m_civicAddress->getPOBOX();};
    const std::optional<std::string> &getADDCODE() const {return m_civicAddress->getADDCODE();};
    const std::optional<std::string> &getSEAT() const {return m_civicAddress->getSEAT();};
    const std::optional<std::string> &getRD() const {return m_civicAddress->getRD();};
    const std::optional<std::string> &getRDSEC() const {return m_civicAddress->getRDSEC();};
    const std::optional<std::string> &getRDBR() const {return m_civicAddress->getRDBR();};
    const std::optional<std::string> &getRDSUBBR() const {return m_civicAddress->getRDSUBBR();};
    const std::optional<std::string> &getPRM() const {return m_civicAddress->getPRM();};
    const std::optional<std::string> &getPOM() const {return m_civicAddress->getPOM();};
    const std::optional<std::string> &getUsageRules() const {return m_civicAddress->getUsageRules();};
    const std::optional<std::string> &getMethod() const {return m_civicAddress->getMethod();};
    const std::optional<std::string> &getProvidedBy() const {return m_civicAddress->getProvidedBy();};

    mb_smf_sc_civic_address_t *populateCivicAddress();

private:
    void populateAddr(char* a[6]) const;
    std::shared_ptr<reftools::mbsf::CivicAddress> m_civicAddress;

};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_CIVIC_ADDRESS_HH_ */
