#ifndef _MBSF_NCGI_HH_
#define _MBSF_NCGI_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS NCGI class
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
#include "openapi/model/Ncgi.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class PlmnId;
}

MBSF_NAMESPACE_START

class MBSNcgi {
public:

    MBSNcgi(fiveg_mag_reftools::CJson &json, bool as_request);
    MBSNcgi(const std::shared_ptr<reftools::mbsf::Ncgi> &ncgi);
    MBSNcgi() = delete;
    MBSNcgi(MBSNcgi &&other) = delete;
    MBSNcgi(const MBSNcgi &other) = delete;
    MBSNcgi &operator=(MBSNcgi &&other) = delete;
    MBSNcgi &operator=(const MBSNcgi &other) = delete;

    virtual ~MBSNcgi();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::Ncgi> &getNcgi() const {return m_ncgi;};
    const std::shared_ptr<reftools::mbsf::PlmnId> &getPlmnId() const {return m_ncgi->getPlmnId();};
    const std::string &getNrCellId() const {return m_ncgi->getNrCellId();};
    const std::optional<std::string > &getNid() const {return m_ncgi->getNid();};

    uint64_t *nid();
    uint64_t nrCellId();
    mb_smf_sc_ncgi_t *populateNcgi();

private:
    std::shared_ptr<reftools::mbsf::Ncgi> m_ncgi;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_NCGI_HH_ */
