#ifndef _MBSF_OBJECT_MANIFEST_HH_
#define _MBSF_OBJECT_MANIFEST_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Object Manifest class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

#include "openapi/model/ObjectManifest.h"
#include "openapi/model/Object.h"

#include "common.hh"

namespace reftools::mbsf {
    class Object;
    class ObjectManifest;
}

MBSF_NAMESPACE_START

class CarouselObject;
class UserDataIngSession;

class ObjManifest {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    ObjManifest(fiveg_mag_reftools::CJson &json, bool as_request);
    ObjManifest(const std::shared_ptr<reftools::mbsf::ObjectManifest> &object_manifest);
    ObjManifest(std::list<std::shared_ptr<CarouselObject >> object); 

    ObjManifest() = delete;
    ObjManifest(ObjManifest &&other) = delete;
    ObjManifest(const ObjManifest &other) = delete;
    ObjManifest &operator=(ObjManifest &&other) = delete;
    ObjManifest &operator=(const ObjManifest &other) = delete;

    virtual ~ObjManifest();

    fiveg_mag_reftools::CJson json(bool as_request) const;

    const std::shared_ptr<reftools::mbsf::ObjectManifest> objectManifest() const {return m_objectManifest;};
    ObjManifest &updateObjectManifest(std::shared_ptr<reftools::mbsf::ObjectManifest> object_manifest);


private:

    void addObjects(std::list<std::shared_ptr<CarouselObject >> objects);
    
    std::shared_ptr<reftools::mbsf::ObjectManifest> m_objectManifest;
    std::unique_ptr<std::recursive_mutex> m_objectManifestMutex;
};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_OBJECT_MANIFEST_HH_ */
