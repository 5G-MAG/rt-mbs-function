/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Service Area class
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
#include "ogs-crypt.h"

// standard template library includes
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "MBSNcgi.hh"
#include "MBSNcgiTai.hh"
#include "TrackingAreaIdentity.hh"
#include "openapi/model/MbsServiceArea.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "ogs-crypt.h"

#include "ServiceArea.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::Ncgi;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::Tai;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

ServiceArea::ServiceArea(CJson &json, bool as_request)
    :m_mbsServiceArea(new MbsServiceArea(json, as_request))
{
}

ServiceArea::ServiceArea(const std::shared_ptr<MbsServiceArea> &mbs_service_area)
    :m_mbsServiceArea(mbs_service_area)
{
}

ServiceArea::~ServiceArea()
{
}

CJson ServiceArea::json(bool as_request = false) const
{
    return m_mbsServiceArea->toJSON(as_request);
}

mb_smf_sc_mbs_service_area_t *ServiceArea::populateServiceArea() {


    mb_smf_sc_mbs_service_area_t *mbs_service_area = mb_smf_sc_mbs_service_area_new();
    ncgiTais(mbs_service_area);
    tais(mbs_service_area);
    intendedServiceAreas(mbs_service_area);
    return mbs_service_area;
}

void ServiceArea::ncgiTais(mb_smf_sc_mbs_service_area_t *mbs_service_area) {

    //std::optional<std::list<std::optional<std::shared_ptr< NcgiTai > >
    const reftools::mbsf::MbsServiceArea::NcgiListType &ncgi_tais = getNcgiList();
    if (!ncgi_tais.has_value()) return;

    ogs_list_init(&mbs_service_area->ncgi_tais);

    for(const auto &ncgi_tai: ncgi_tais.value()) {
        if (ncgi_tai.has_value()) {
            std::shared_ptr< MBSNcgiTai > mbs_ncgi_tai = nullptr;
            std::shared_ptr< NcgiTai > recv_ncgi_tai = ncgi_tai.value();
            mbs_ncgi_tai.reset(new MBSNcgiTai(recv_ncgi_tai));
            mb_smf_sc_ncgi_tai_t *mb_smf_ncgi_tai = mbs_ncgi_tai->populateNcgiTai();
            ogs_list_add(&mbs_service_area->ncgi_tais, mb_smf_ncgi_tai);
        }

    }
}

void ServiceArea::tais(mb_smf_sc_mbs_service_area_t *mbs_service_area) {

    //std::optional<std::list<std::optional<std::shared_ptr< Tai > >
    const reftools::mbsf::MbsServiceArea::TaiListType &tais = getTaiList();
    if (!tais.has_value()) return;

    ogs_list_init(&mbs_service_area->tais);

    for(const auto &tai: tais.value()) {
        if (tai.has_value()) {
            std::shared_ptr< TrackingAreaIdentity > tracking_area_identity = nullptr;
            std::shared_ptr< Tai > recv_tai = tai.value();
            tracking_area_identity.reset(new TrackingAreaIdentity(recv_tai));
            mb_smf_sc_tai_t *mb_smf_tai = tracking_area_identity->populateTai();
            ogs_list_add(&mbs_service_area->tais, mb_smf_tai);
        }

    }
}



// The generated model keeps an attribute of format "byte" as the base64 text it came as (TS 29.571 IntendedServiceArea: "Bytes", base64 of the
// octets of the Intended Service Area Coordinates IE); the library type holds the octets.
static bool base64ToOctets(const std::basic_string<unsigned char> &text, std::vector<uint8_t> &octets)
{
    const std::string in(text.begin(), text.end());
    const int size = ogs_base64_decoded_size(in.c_str());
    if (size <= 0) return false;
    octets.assign(static_cast<size_t>(size), 0);
    const int len = ogs_base64_decode_to_buffer(octets.data(), octets.size(), in.c_str());
    if (len <= 0) return false;
    octets.resize(static_cast<size_t>(len));
    return true;
}

static bool octetsToBase64(const uint8_t *octets, size_t len, std::basic_string<unsigned char> &text)
{
    const int size = ogs_base64_encoded_size(len);
    if (size <= 0) return false;
    std::string out(static_cast<size_t>(size), '\0');
    const int n = ogs_base64_encode_from_buffer(out.data(), out.size(), octets, len);
    if (n <= 0) return false;
    out.resize(strlen(out.c_str()));
    text.assign(out.begin(), out.end());
    return true;
}

// NR NTN Intended Service Areas (TS 29.571 clause 5.9.4.4): each is the octets of the Intended Service Area Coordinates IE of TS 38.413
// clause 9.3.1.273, handed on to the MB-SMF as they are (the MBSF does not interpret them).
void ServiceArea::intendedServiceAreas(mb_smf_sc_mbs_service_area_t *mbs_service_area) {

    const reftools::mbsf::MbsServiceArea::IntendedServAreaListType &areas = getIntendedServAreaList();
    if (!areas.has_value()) return;

    for (const auto &area: areas.value()) {
        if (!area.has_value() || area.value().empty()) continue;
        std::vector<uint8_t> octets;
        if (!base64ToOctets(area.value(), octets) ||
            !mb_smf_sc_mbs_service_area_add_intended_service_area(mbs_service_area, octets.data(), octets.size())) {
            ogs_error("Could not add an NR NTN Intended Service Area to the MBS Service Area");
        }
    }
}

std::shared_ptr<MbsServiceArea> ServiceArea::fromServiceArea(const mb_smf_sc_mbs_service_area_t *area)
{
    if (!area) return nullptr;

    std::shared_ptr<MbsServiceArea> result(new MbsServiceArea());

    mb_smf_sc_ncgi_tai_t *ncgi_tai;
    ogs_list_for_each(&area->ncgi_tais, ncgi_tai) {
        std::shared_ptr<NcgiTai> entry = MBSNcgiTai::fromNcgiTai(ncgi_tai);
        if (entry) result->addNcgiList(entry);
    }

    mb_smf_sc_tai_t *tai;
    ogs_list_for_each(&area->tais, tai) {
        std::shared_ptr<Tai> entry = TrackingAreaIdentity::fromTai(tai);
        if (entry) result->addTaiList(entry);
    }

    mb_smf_sc_intended_service_area_t *isa;
    ogs_list_for_each(&area->intended_service_areas, isa) {
        if (isa->coordinates && isa->coordinates_len > 0) {
            std::basic_string<unsigned char> text;
            if (octetsToBase64(isa->coordinates, isa->coordinates_len, text)) result->addIntendedServAreaList(text);
        }
    }

    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

