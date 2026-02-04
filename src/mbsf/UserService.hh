#ifndef _MBSF_USER_SERVICE_HH_
#define _MBSF_USER_SERVICE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS USER SERVICE class
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

#include <chrono>
#include <memory>
#include "openapi/model/MBSUserService.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

MBSF_NAMESPACE_START

class Open5GSEvent;

class UserService {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;
    UserService(fiveg_mag_reftools::CJson &json, bool as_request);
    UserService(const std::shared_ptr<reftools::mbsf::MBSUserService> &mbs_user_service);
    UserService() = delete;
    UserService(UserService &&other) = delete;
    UserService(const UserService &other) = delete;
    UserService &operator=(UserService &&other) = delete;
    UserService &operator=(const UserService &other) = delete;

    virtual ~UserService();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    static const std::shared_ptr<UserService> &find(const std::string &id); // throws std::out_of_range if id does not exist
    void update(fiveg_mag_reftools::CJson &json, bool as_request);
    const std::string &userServiceId() const { return m_UserServiceId; };
    const std::shared_ptr<reftools::mbsf::MBSUserService> &getMBSUserService() const {return m_MBSUserService;};
    const std::string &getMBSUserServiceType() const;
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};

    static bool processEvent(Open5GSEvent &event);

private:
    std::shared_ptr<reftools::mbsf::MBSUserService> m_MBSUserService;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_UserServiceId;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_SERVICE_HH_ */
