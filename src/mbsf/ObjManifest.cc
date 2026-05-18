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
#include "CarouselObject.hh"

#include "openapi/model/Object.h"
#include "openapi/model/ObjectManifest.h"

// Header include for this class
#include "ObjManifest.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::Object;
using reftools::mbsf::ObjectManifest;

MBSF_NAMESPACE_START

ObjManifest::ObjManifest(CJson &json, bool as_request)
    :m_objectManifest(new ObjectManifest(json, as_request))
    ,m_objectLocators()
    ,m_objectManifestMutex(new std::recursive_mutex)
{
}

ObjManifest::ObjManifest(const std::shared_ptr<reftools::mbsf::ObjectManifest> &object_manifest)
    :m_objectManifest(object_manifest)
    ,m_objectLocators()
    ,m_objectManifestMutex(new std::recursive_mutex)
{
}

ObjManifest::ObjManifest(const std::shared_ptr<reftools::mbsf::ObjectManifest> &object_manifest, const std::list<std::string> &object_locators)
    :m_objectManifest(object_manifest)
    ,m_objectLocators(object_locators)
    ,m_objectManifestMutex(new std::recursive_mutex)
{

}

ObjManifest::ObjManifest(std::list<std::shared_ptr<CarouselObject >> objects)
    :m_objectManifest(new ObjectManifest())
    ,m_objectLocators()
    ,m_objectManifestMutex(new std::recursive_mutex)
{
    addObjects(objects);
}

ObjManifest::ObjManifest(std::list<std::shared_ptr<CarouselObject >> objects, const std::list<std::string> &object_locators)
    :m_objectManifest(new ObjectManifest())
    ,m_objectLocators(object_locators)
    ,m_objectManifestMutex(new std::recursive_mutex)

{
    addObjects(objects);
}

ObjManifest::~ObjManifest()
{
}

CJson ObjManifest::json(bool as_request = false) const
{
    return m_objectManifest->toJSON(as_request);
}

ObjManifest &ObjManifest::updateObjectManifest(std::shared_ptr<ObjectManifest> object_manifest)
{
    std::lock_guard<decltype(m_objectManifestMutex)::element_type> lock(*m_objectManifestMutex);
    m_objectManifest = object_manifest;
    return *this;
}

void ObjManifest::addObjects(std::list<std::shared_ptr<CarouselObject >> objects)
{
    std::lock_guard<decltype(m_objectManifestMutex)::element_type> lock(*m_objectManifestMutex);
    for(const auto &obj : objects) {
        if(!obj) continue;
	const std::shared_ptr<reftools::mbsf::Object> object = obj->object();
	if(!object) continue;
	m_objectManifest->addObjects(std::move(object));
    }
}

void ObjManifest::forEachObject(std::function<void(const std::string &)> fn)
{
    std::lock_guard<decltype(m_objectManifestMutex)::element_type> lock(*m_objectManifestMutex);
    for (auto &object_locator : m_objectLocators) {
        fn(object_locator);
    }
}

std::list<std::string> ObjManifest::getObjectLocators() {
    std::lock_guard<decltype(m_objectManifestMutex)::element_type> lock(*m_objectManifestMutex);
    return m_objectLocators;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

