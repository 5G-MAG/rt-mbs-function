/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Media Info class
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
#include "UserDataIngSession.hh"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsMediaInfo.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/ObjectDistrMethInfo.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "MediaInfo.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsMediaInfo;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MediaType;

MBSF_NAMESPACE_START

MediaInfo::MediaInfo(CJson &json, bool as_request)
    :m_mbsMediaInfo(new MbsMediaInfo(json, as_request))
{
}

MediaInfo::MediaInfo(const std::shared_ptr<MbsMediaInfo> &mbs_media_info)
    :m_mbsMediaInfo(mbs_media_info)
{
}

MediaInfo::~MediaInfo()
{
}

CJson MediaInfo::json(bool as_request = false) const
{
    return m_mbsMediaInfo->toJSON(as_request);
}

const std::unordered_map<std::string, mb_smf_sc_mbs_media_type_e>& MediaInfo::mediaType() {
    static const std::unordered_map<std::string, mb_smf_sc_mbs_media_type_e> m = {
        {"AUDIO", MEDIA_TYPE_AUDIO},
        {"VIDEO", MEDIA_TYPE_VIDEO},
        {"DATA", MEDIA_TYPE_DATA},
        {"APPLICATION", MEDIA_TYPE_APPLICATION},
        {"CONTROL", MEDIA_TYPE_CONTROL},
        {"TEXT", MEDIA_TYPE_TEXT},
        {"MESSAGE", MEDIA_TYPE_MESSAGE},
        {"OTHER", MEDIA_TYPE_OTHER}
    };
    return m;
}

#if 0
const std::unordered_map<std::string, mb_smf_sc_mbs_media_type_e> MediaInfo::mediaType = {
        // MB-SMF Media type mapping
    { "AUDIO", MEDIA_TYPE_AUDIO},
    { "VIDEO", MEDIA_TYPE_VIDEO},
    { "DATA", MEDIA_TYPE_DATA},
    { "APPLICATION", MEDIA_TYPE_APPLICATION},
    { "CONTROL", MEDIA_TYPE_CONTROL},
    { "TEXT", MEDIA_TYPE_TEXT},
    { "MESSAGE", MEDIA_TYPE_MESSAGE},
    { "OTHER", MEDIA_TYPE_OTHER}

};

#endif
void MediaInfo::codecs(mb_smf_sc_mbs_media_info_t *media_info) {
    //std::optional<std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > > CodecsType;
    reftools::mbsf::MbsMediaInfo::CodecsType codecs = m_mbsMediaInfo->getCodecs();
    if (!codecs.has_value()) return;
    int i = 0;
    for(const auto &codec: codecs.value()) {
        if (codec.has_value()) {
            const std::string &media_codec = codec.value();
            media_info->codecs[i++] = ogs_strdup(media_codec.c_str());
        }
        if (i>2) break;
    }

}

mb_smf_sc_mbs_media_info_t *MediaInfo::populateMediaInfo() {
    mb_smf_sc_mbs_media_info_t *media_info = mb_smf_sc_mbs_media_info_new();
    media_info->mbs_media_type = lookup();
    media_info->max_requested_mbs_bandwidth_downlink = bandwidth(getMaximumReqBandwidthDownlink());
    media_info->min_requested_mbs_bandwidth_downlink = bandwidth(getMinimumReqBandwidthDownlink());
    codecs(media_info);
    return media_info;
}

mb_smf_sc_mbs_media_type_e MediaInfo::lookup() {
  const auto &map = MediaInfo::mediaType();
  const std::optional<std::shared_ptr< MediaType > > &media_type = getMediaType();
  if (!media_type.has_value()) return MEDIA_TYPE_NULL;
  const std::shared_ptr< MediaType > &mbs_media_type = media_type.value();
  const std::string &type = mbs_media_type->getString();
  auto it = map.find(type);
  if (it == map.end()) return MEDIA_TYPE_NULL;

  return it->second;
}
/*
uint64_t* MediaInfo::bandwidth(const std::optional<std::string>& bw) {
    if (!bw.has_value()) return nullptr;
    try {
        return new uint64_t(std::stoull(*bw));
    } catch (...) {
        return nullptr;
    }
}
*/

uint64_t* MediaInfo::bandwidth(const std::optional<std::string>& bw) {

    if (!bw.has_value()) return nullptr;
    std::string bwidth = bw.value();

    // Trim leading/trailing spaces
    bwidth.erase(0, bwidth.find_first_not_of(" \t"));
    bwidth.erase(bwidth.find_last_not_of(" \t") + 1);

    // Extract number and unit
    std::stringstream ss(bwidth);
    double value;
    std::string unit;
    ss >> value >> unit;

    // Normalize unit to lowercase
    std::transform(unit.begin(), unit.end(), unit.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    uint64_t multiplier = 1;

    if (unit == "bps") {
        multiplier = 1ULL;
    } else if (unit == "kbps") {
        multiplier = 1000ULL;
    } else if (unit == "mbps") {
        multiplier = 1000ULL * 1000ULL;
    } else if (unit == "gbps") {
        multiplier = 1000ULL * 1000ULL * 1000ULL;
    } else if (unit == "tbps") {
        multiplier = 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else {
        throw std::invalid_argument("Unknown unit: " + unit);
    }

    uint64_t *band_width = new uint64_t(static_cast<uint64_t>(value * multiplier));
    return band_width;
}


#if 0
typedef struct mb_smf_sc_mbs_media_info_s {
    mb_smf_sc_mbs_media_type_e mbs_media_type;
    uint64_t *max_requested_mbs_bandwidth_downlink;
    uint64_t *min_requested_mbs_bandwidth_downlink;
    char *codecs[2];
} mb_smf_sc_mbs_media_info_t;

typedef enum {
        NO_VAL,
        VAL_AUDIO,
        VAL_VIDEO,
        VAL_DATA,
        VAL_APPLICATION,
        VAL_CONTROL,
        VAL_TEXT,
        VAL_MESSAGE,
        VAL_OTHER,

        OTHER
    } Enum;

typedef enum {
    MEDIA_TYPE_NULL = 0,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_DATA,
    MEDIA_TYPE_APPLICATION,
    MEDIA_TYPE_CONTROL,
    MEDIA_TYPE_TEXT,
    MEDIA_TYPE_MESSAGE,
    MEDIA_TYPE_OTHER
} mb_smf_sc_mbs_media_type_e;
#endif

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

