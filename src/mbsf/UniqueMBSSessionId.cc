/******************************************************************************
 * 5G-MAG Reference Tools: MBS: Unique MBS Session Id
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
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

#include <algorithm>
#include <list>
#include <memory>
#include <set>
#include <string>

#include "common.hh"
#include "openapi/model/CivicAddress.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/GeographicArea.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/Ipv6Addr.h"
#include "openapi/model/Ipv6Prefix.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/Ncgi.h"
#include "openapi/model/NcgiTai.h"
#include "openapi/model/PlmnId.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/Tai.h"
#include "openapi/model/Tmgi.h"

#include "UniqueMBSSessionId.hh"

using fiveg_mag_reftools::ModelObject;
using reftools::mbsf::CivicAddress;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::GeographicArea;
using reftools::mbsf::IpAddr;
using reftools::mbsf::Ipv6Addr;
using reftools::mbsf::Ipv6Prefix;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::Ncgi;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::PlmnId;
using reftools::mbsf::Ssm;
using reftools::mbsf::Tai;
using reftools::mbsf::Tmgi;

namespace std {
template <>
struct hash<IpAddr> {
    std::size_t operator()(const IpAddr &ip_addr) const;
};

template <>
struct hash<Ipv6Addr> {
    std::size_t operator()(const Ipv6Addr &ip_addr) const;
};

template <>
struct hash<Ipv6Prefix> {
    std::size_t operator()(const Ipv6Prefix &prefix) const;
};

template <>
struct hash<PlmnId> {
    std::size_t operator()(const PlmnId &plmn_id) const;
};

template <>
struct hash<Ssm> {
    std::size_t operator()(const Ssm &ssm) const;
};

template <>
struct hash<Tmgi> {
    std::size_t operator()(const Tmgi &tmgi) const;
};

template <>
struct hash<MbsSessionId> {
    std::size_t operator()(const MbsSessionId &mbs_session_id) const;
};

template <>
struct hash<MbsServiceArea> {
    std::size_t operator()(const MbsServiceArea &mbs_svc_area) const;
};

template <>
struct hash<ExternalMbsServiceArea> {
    std::size_t operator()(const ExternalMbsServiceArea &ext_mbs_svc_area) const;
};

template <>
struct hash<NcgiTai> {
    std::size_t operator()(const NcgiTai &ncgi_tai) const;
};

template <>
struct hash<Tai> {
    std::size_t operator()(const Tai &tai) const;
};

template <>
struct hash<Ncgi> {
    std::size_t operator()(const Ncgi &ncgi) const;
};

template <>
struct hash<GeographicArea> {
    std::size_t operator()(const GeographicArea &geog_area) const;
};

template <>
struct hash<CivicAddress> {
    std::size_t operator()(const CivicAddress &civic_addr) const;
};

std::size_t hash<IpAddr>::operator()(const IpAddr &ip_addr) const
{
    std::size_t result{0x8b6fdf2af3de8565}; // random number to initialise
    auto &ipv4 = ip_addr.getIpv4Addr();
    if (ipv4) {
        std::hash<std::string> str_hash;
        result ^= str_hash(ipv4.value());
    }
    auto &ipv6 = ip_addr.getIpv6Addr();
    if (ipv6) {
        std::hash<Ipv6Addr> ipv6_hash;
        result ^= ipv6_hash(*ipv6.value());
    }
    auto &v6_prefix = ip_addr.getIpv6Prefix();
    if (v6_prefix) {
        std::hash<Ipv6Prefix> v6_prefix_hash;
        result ^= v6_prefix_hash(*v6_prefix.value());
    }
    return result;
}

std::size_t hash<Ipv6Addr>::operator()(const Ipv6Addr &ip_addr) const
{
    std::size_t result{0x872c51cff3801538}; // random number to initialise
    result ^= std::hash<std::string>{}(ip_addr);
    return result;
}

std::size_t hash<Ipv6Prefix>::operator()(const Ipv6Prefix &prefix) const
{
    std::size_t result{0x1142047a5d2cfa55}; // random number to initialise
    result ^= std::hash<std::string>{}(prefix);
    return result;
}

std::size_t hash<MbsSessionId>::operator()(const MbsSessionId &mbs_session_id) const
{
    std::size_t result{0x78a115e3d0fe74fe}; // random number to initialise
    auto &ssm = mbs_session_id.getSsm();
    if (ssm) {
        std::hash<Ssm> ssm_hash;
        result ^= ssm_hash(*ssm.value());
    } else {
        auto &tmgi = mbs_session_id.getTmgi();
        auto &nid = mbs_session_id.getNid();
        if (tmgi) {
            std::hash<Tmgi> tmgi_hash;
            result ^= tmgi_hash(*tmgi.value());
        }
        if (nid) {
            std::hash<std::string> nid_hash;
            result ^= nid_hash(nid.value());
        }
    }
    return result;
}

std::size_t hash<PlmnId>::operator()(const PlmnId &plmn_id) const {
    std::size_t result{0x615bf625c3fa4c28}; // random number to initialise
    std::hash<std::string> str_hash;
    result ^= str_hash(plmn_id.getMcc());
    result ^= str_hash(plmn_id.getMnc());
    return result;
}

std::size_t hash<Ssm>::operator()(const Ssm &ssm) const
{
    std::size_t result{0x5087f114cc282568}; // random number to initialise
    std::hash<IpAddr> ip_addr_hash;
    result ^= ip_addr_hash(*ssm.getSourceIpAddr());
    result ^= (ip_addr_hash(*ssm.getDestIpAddr()) << 1);
    return result;
}

std::size_t hash<Tmgi>::operator()(const Tmgi &tmgi) const
{
    std::size_t result{0x9849803b31e2f994}; // random number to initialise
    std::hash<std::string> str_hash;
    result ^= str_hash(tmgi.getMbsServiceId());
    std::hash<PlmnId> plmn_id_hash;
    result ^= plmn_id_hash(*tmgi.getPlmnId());
    return result;
}

std::size_t hash<MbsServiceArea>::operator()(const MbsServiceArea &mbs_svc_area) const
{
    std::size_t result{0xad219eb435d47384}; // random number to initialise

    const auto &ncgi_tais = mbs_svc_area.getNcgiList();
    if (ncgi_tais) {
        std::hash<NcgiTai> ncgi_tai_hash;
        for (const auto &ncgi_tai_item : ncgi_tais.value()) {
            if (ncgi_tai_item) {
                result ^= ncgi_tai_hash(*ncgi_tai_item.value());
            }
        }
    }

    const auto &tais = mbs_svc_area.getTaiList();
    if (tais) {
        std::hash<Tai> tai_hash;
        for (const auto &tai_item : tais.value()) {
            if (tai_item) {
                result ^= tai_hash(*tai_item.value());
            }
        }
    }

    return result;
}

std::size_t hash<ExternalMbsServiceArea>::operator()(const ExternalMbsServiceArea &ext_mbs_svc_area) const
{
    std::size_t result{0x265ebcfc204074b4}; // random number to initialise

    const auto &geog_areas = ext_mbs_svc_area.getGeographicAreaList();
    if (geog_areas) {
        std::hash<GeographicArea> geog_hash;
        for (const auto &geog_area_item : geog_areas.value()) {
            if (geog_area_item) {
                result ^= geog_hash(*geog_area_item.value());
            }
        }
    }

    const auto &civic_addrs  = ext_mbs_svc_area.getCivicAddressList();
    if (civic_addrs) {
        std::hash<CivicAddress> civic_hash;
        for (const auto &civic_addr_item : civic_addrs.value()) {
            if (civic_addr_item) {
                result ^= civic_hash(*civic_addr_item.value());
            }
        }
    }

    return result;
}

std::size_t hash<NcgiTai>::operator()(const NcgiTai &ncgi_tai) const
{
    std::size_t result{0x02197b77af2ab3b9}; // random number to initialise

    std::hash<Tai> tai_hash;
    result ^= tai_hash(*ncgi_tai.getTai());

    std::hash<Ncgi> ncgi_hash;
    for (const auto &ncgi_item : ncgi_tai.getCellList()) {
        if (ncgi_item) {
            result ^= ncgi_hash(*ncgi_item.value());
        }
    }

    return result;
}

std::size_t hash<Tai>::operator()(const Tai &tai) const
{
    std::size_t result{0x8afd55ce156d1ea4}; // random number to initialise

    std::hash<PlmnId> plmn_id_hash;
    result ^= plmn_id_hash(*tai.getPlmnId());

    std::hash<std::string> str_hash;
    result ^= str_hash(tai.getTac());

    const auto &nid = tai.getNid();
    if (nid) {
        result ^= (str_hash(nid.value())<<1);
    }

    return result;
}

std::size_t hash<Ncgi>::operator()(const Ncgi &ncgi) const
{
    std::size_t result{0x6422247acdda7f6b}; // random number to initialise

    std::hash<PlmnId> plmn_id_hash;
    result ^= plmn_id_hash(*ncgi.getPlmnId());

    std::hash<std::string> str_hash;
    result ^= str_hash(ncgi.getNrCellId());

    const auto &nid = ncgi.getNid();
    if (nid) {
        result ^= (str_hash(nid.value())<<1);
    }

    return result;
}

std::size_t hash<GeographicArea>::operator()(const GeographicArea &geog_area) const
{
    std::size_t result{0xe3fd54301ab3ab54}; // random number to initialise

    std::hash<std::string> str_hash;
    result ^= str_hash(geog_area.getShape()->getString());

    // TODO: fill this in.

    return result;
}

std::size_t hash<CivicAddress>::operator()(const CivicAddress &civic_addr) const
{
    std::size_t result{0xb53ab826408c4592}; // random number to initialise

    std::hash<std::optional<std::string> > str_hash;

    result ^= str_hash(civic_addr.getCountry());
    result ^= str_hash(civic_addr.getA1())*2;
    result ^= str_hash(civic_addr.getA2())*3;
    result ^= str_hash(civic_addr.getA3())*5;
    result ^= str_hash(civic_addr.getA4())*7;
    result ^= str_hash(civic_addr.getA5())*11;
    result ^= str_hash(civic_addr.getA6())*13;
    result ^= str_hash(civic_addr.getPRD())*17;
    result ^= str_hash(civic_addr.getPOD())*19;
    result ^= str_hash(civic_addr.getSTS())*23;
    result ^= str_hash(civic_addr.getHNO())*29;
    result ^= str_hash(civic_addr.getHNS())*31;
    result ^= str_hash(civic_addr.getLMK())*37;
    result ^= str_hash(civic_addr.getLOC())*41;
    result ^= str_hash(civic_addr.getNAM())*43;
    result ^= str_hash(civic_addr.getPC())*47;
    result ^= str_hash(civic_addr.getBLD())*53;
    result ^= str_hash(civic_addr.getUNIT())*59;
    result ^= str_hash(civic_addr.getFLR())*61;
    result ^= str_hash(civic_addr.getROOM())*67;
    result ^= str_hash(civic_addr.getPLC())*71;
    result ^= str_hash(civic_addr.getPCN())*73;
    result ^= str_hash(civic_addr.getPOBOX())*79;
    result ^= str_hash(civic_addr.getADDCODE())*83;
    result ^= str_hash(civic_addr.getSEAT())*89;
    result ^= str_hash(civic_addr.getRD())*97;
    result ^= str_hash(civic_addr.getRDSEC())*101;
    result ^= str_hash(civic_addr.getRDBR())*103;
    result ^= str_hash(civic_addr.getRDSUBBR())*107;
    result ^= str_hash(civic_addr.getPRM())*109;
    result ^= str_hash(civic_addr.getPOM())*113;
    result ^= str_hash(civic_addr.getUsageRules())*127;
    result ^= str_hash(civic_addr.getMethod())*131;
    result ^= str_hash(civic_addr.getProvidedBy())*137;

    return result;
}

std::size_t hash<MBSF_NAMESPACE_NAME(UniqueMbsSessionId)>::operator()(const MBSF_NAMESPACE_NAME(UniqueMbsSessionId) &mbs_session_id) const
{
    return mbs_session_id.hash();
}

} // End namespace std

namespace {
    struct PlmnIdLess {
        bool operator()(const PlmnId &a, const PlmnId &b) const {
            if (a.getMcc() != b.getMcc()) return a.getMcc() < b.getMcc();
            return a.getMnc() < b.getMnc();
        };
    };

    struct NcgiLess {
        bool operator()(const Ncgi &a, const Ncgi &b) const {
            if (*a.getPlmnId() != *b.getPlmnId()) return PlmnIdLess()(*a.getPlmnId(), *b.getPlmnId());
            if (a.getNrCellId() != b.getNrCellId()) return a.getNrCellId() < b.getNrCellId();
            return a.getNid() < b.getNid();
        };
        bool operator()(const std::shared_ptr<Ncgi> &a, const std::shared_ptr<Ncgi> &b) const {
            if (!a && b) return true;
            if (!b && a) return false;
            if (a) return operator()(*a, *b);
            return false;
        };
        bool operator()(const std::optional<std::shared_ptr<Ncgi> > &a, const std::optional<std::shared_ptr<Ncgi> > &b) const {
            if (!a && b) return true;
            if (!b && a) return false;
            if (a) return operator()(a.value(), b.value());
            return false;
        };
    };

    struct TaiLess {
        bool operator()(const Tai &a, const Tai &b) const {
            if (*a.getPlmnId() != *b.getPlmnId()) return PlmnIdLess()(*a.getPlmnId(), *b.getPlmnId());
            if (a.getTac() != b.getTac()) return a.getTac() < b.getTac();
            return a.getNid() < b.getNid();
        };
        bool operator()(const std::shared_ptr<Tai> &a, const std::shared_ptr<Tai> &b) const {
            if (!a && b) return true;
            if (a && !b) return false;
            if (!a) return false;
            return operator()(*a, *b);
        };
        bool operator()(const std::optional<std::shared_ptr<Tai> > &a, const std::optional<std::shared_ptr<Tai> > &b) const {
            if (!a && b) return true;
            if (a && !b) return false;
            if (!a) return false;
            return operator()(a.value(), b.value());
        };
    };

    struct NcgiTaiLess {
        bool operator()(const NcgiTai &a, const NcgiTai &b) const {
            if (*a.getTai() != *b.getTai()) return TaiLess()(*a.getTai(), *b.getTai());

            auto a_ncgi_ordered = a.getCellList();
            auto b_ncgi_ordered = b.getCellList();
            a_ncgi_ordered.sort(NcgiLess());
            b_ncgi_ordered.sort(NcgiLess());

            for (auto a_it = a_ncgi_ordered.begin(), b_it = b_ncgi_ordered.begin();
                 a_it != a_ncgi_ordered.end() && b_it != b_ncgi_ordered.end(); ) {
                if (a_it == a_ncgi_ordered.end()) return true;
                if (b_it == b_ncgi_ordered.end()) return false;
                if (*a_it != *b_it) return NcgiLess()(*a_it, *b_it);
                if (a_it != a_ncgi_ordered.end()) a_it++;
                if (b_it != b_ncgi_ordered.end()) b_it++;
            }
            return false;
        };
        bool operator()(const std::shared_ptr<NcgiTai> &a, const std::shared_ptr<NcgiTai> &b) const {
            if (!a && b) return true;
            if (a && !b) return false;
            if (!a) return false;
            return operator()(*a, *b);
        };
        bool operator()(const std::optional<std::shared_ptr<NcgiTai> > &a, const std::optional<std::shared_ptr<NcgiTai> > &b) const {
            if (!a && b) return true;
            if (a && !b) return false;
            if (!a) return false;
            return operator()(a.value(), b.value());
        };
    };
}

MBSF_NAMESPACE_START

static bool svc_area_overlap(const std::shared_ptr<MbsServiceArea> &a, const std::shared_ptr<MbsServiceArea> &b);
static bool ext_svc_area_overlap(const std::shared_ptr<ExternalMbsServiceArea> &a, const std::shared_ptr<ExternalMbsServiceArea> &b);
static std::strong_ordering mbs_session_ordering(bool a_tmgi_req, const std::shared_ptr<MbsSessionId> &a_mbs_session_id, bool b_tmgi_req, const std::shared_ptr<MbsSessionId> &b_mbs_session_id);
static std::strong_ordering svc_area_ordering(const std::shared_ptr<MbsServiceArea> &a, const std::shared_ptr<MbsServiceArea> &b);
static std::strong_ordering ext_svc_area_ordering(const std::shared_ptr<ExternalMbsServiceArea> &a, const std::shared_ptr<ExternalMbsServiceArea> &b);
static std::strong_ordering ssm_ordering(const std::optional<std::shared_ptr<Ssm> > &a, const std::optional<std::shared_ptr<Ssm> > &b);
static std::strong_ordering tmgi_ordering(const Tmgi &a, const Tmgi &b);
static std::strong_ordering ip_addr_ordering(const IpAddr &a, const IpAddr &b);

std::string repr(const IpAddr &ip_addr)
{
    std::string result{"IpAddr["};

    auto &ipv4 = ip_addr.getIpv4Addr();
    if (ipv4) {
        result += "\"" + ipv4.value() + "\"";
    }
    auto &ipv6 = ip_addr.getIpv6Addr();
    if (ipv6) {
        result += "\"" + *ipv6.value() + "\"";
    }
    auto &prefix = ip_addr.getIpv6Prefix();
    if (prefix) {
        result += "\"" + *prefix.value() + "\"";
    }

    result += "]";
    return result;
}

std::string repr(const PlmnId &plmn_id)
{
    std::string result{"PLMN-ID[mcc = \""};

    result += plmn_id.getMcc() + "\", mnc = \"" + plmn_id.getMnc() + "\"]";

    return result;
}

std::string repr(const Ssm &ssm)
{
    std::string result{"SSM[sourceIpAddr = "};

    result += repr(*ssm.getSourceIpAddr()) + ", destIpAddr = " + repr(*ssm.getDestIpAddr()) + "]";

    return result;
}

std::string repr(const Tmgi &tmgi)
{
    std::string result{"TMGI[mbsServiceId = \""};

    result += tmgi.getMbsServiceId() + "\", plmnId = " + repr(*tmgi.getPlmnId()) + "]";

    return result;
}

std::string repr(const MbsSessionId &mbs_session_id)
{
    std::string result{"MbsSessionId["};
    auto &ssm = mbs_session_id.getSsm();
    if (ssm) {
        result += "ssm = " + repr(*ssm.value());
    } else {
        auto &tmgi = mbs_session_id.getTmgi();
        if (tmgi) {
            result += "tmgi = " + repr(*tmgi.value());
            auto &nid = mbs_session_id.getNid();
            if (nid) {
                result += ", nid = \"" + nid.value() + "\"";
            }
        }
    }
    result += "]";
    return result;
}

std::string repr(const MbsServiceArea &mbs_session_id)
{
    std::string result{"MbsServiceArea["};
    /* TODO: NcgiTai list */
    /* TODO: Tai list */
    result += "]";
    return result;
}

std::string repr(const ExternalMbsServiceArea &ext_mbs_svc_area)
{
    std::string result{"ExternalMbsServiceArea["};
    /* TODO: Geographic Area list */
    /* TODO: Civic Address list */
    result += "]";
    return result;
}

UniqueMbsSessionId::UniqueMbsSessionId()
    :m_tmgiRequested(false)
    ,m_mbsSessionId(nullptr)
    ,m_mbsServiceArea(nullptr)
    ,m_extMBSServiceArea(nullptr)
{
}

UniqueMbsSessionId::UniqueMbsSessionId(bool tmgi_requested, const std::shared_ptr<reftools::mbsf::MbsSessionId> &mbs_session_id,
                       const std::shared_ptr<reftools::mbsf::MbsServiceArea> &mbs_service_area,
                       const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &ext_mbs_service_area)
    :m_tmgiRequested(tmgi_requested)
    ,m_mbsSessionId(mbs_session_id)
    ,m_mbsServiceArea(mbs_service_area)
    ,m_extMBSServiceArea(ext_mbs_service_area)
{
}

UniqueMbsSessionId::UniqueMbsSessionId(UniqueMbsSessionId &&other)
    :m_tmgiRequested(other.m_tmgiRequested)
    ,m_mbsSessionId(std::move(other.m_mbsSessionId))
    ,m_mbsServiceArea(std::move(other.m_mbsServiceArea))
    ,m_extMBSServiceArea(std::move(other.m_extMBSServiceArea))
{
}

UniqueMbsSessionId::UniqueMbsSessionId(const UniqueMbsSessionId &other)
    :m_tmgiRequested(other.m_tmgiRequested)
    ,m_mbsSessionId(other.m_mbsSessionId)
    ,m_mbsServiceArea(other.m_mbsServiceArea)
    ,m_extMBSServiceArea(other.m_extMBSServiceArea)
{
}

UniqueMbsSessionId &UniqueMbsSessionId::operator=(UniqueMbsSessionId &&other)
{
    m_tmgiRequested = other.m_tmgiRequested;
    m_mbsSessionId = std::move(other.m_mbsSessionId);
    m_mbsServiceArea = std::move(other.m_mbsServiceArea);
    m_extMBSServiceArea = std::move(other.m_extMBSServiceArea);

    return *this;
}

UniqueMbsSessionId &UniqueMbsSessionId::operator=(const UniqueMbsSessionId &other)
{
    m_tmgiRequested = other.m_tmgiRequested;
    m_mbsSessionId = other.m_mbsSessionId;
    m_mbsServiceArea = other.m_mbsServiceArea;
    m_extMBSServiceArea = other.m_extMBSServiceArea;

    return *this;
}

UniqueMbsSessionId::~UniqueMbsSessionId()
{
}

std::strong_ordering UniqueMbsSessionId::operator<=>(const UniqueMbsSessionId &other) const
{
    bool this_null = !m_mbsSessionId;
    bool other_null = !other.m_mbsSessionId;

    /* if both empty values, treat as equal */
    if (this_null && other_null) return std::strong_ordering::equal;

    /* if one empty and the other not, treat empty one as less */
    if (this_null) return std::strong_ordering::less;
    if (other_null) return std::strong_ordering::greater;

    if ((!m_mbsServiceArea && !m_extMBSServiceArea) || /* this is global or... */
        (!other.m_mbsServiceArea && !other.m_extMBSServiceArea) || /* other is global or... */
        (!!m_mbsServiceArea && !!other.m_mbsServiceArea && svc_area_overlap(m_mbsServiceArea, other.m_mbsServiceArea)) || /* MBS Service Areas overlap or... */
        (!!m_extMBSServiceArea && !!other.m_extMBSServiceArea && ext_svc_area_overlap(m_extMBSServiceArea, other.m_extMBSServiceArea)) /* External MBS Service Areas overlap */
       ) {

        /* areas are equivalent check the mbs session id */
        return mbs_session_ordering(m_tmgiRequested, m_mbsSessionId, other.m_tmgiRequested, other.m_mbsSessionId);
    }

    /* if non-matching MBS Service Areas, order by MBS Service Areas */
    if (!!m_mbsServiceArea && !!other.m_mbsServiceArea) {
        return svc_area_ordering(m_mbsServiceArea, other.m_mbsServiceArea);
    }

    /* if non-matching External Service Areas, order by External Service Areas */
    if (!!m_extMBSServiceArea && !!other.m_extMBSServiceArea) {
        return ext_svc_area_ordering(m_extMBSServiceArea, other.m_extMBSServiceArea);
    }

    /* If we have MBS service areas, but other only has external service areas, then we are greater (just for ordering) */
    if (!!m_mbsServiceArea) {
        return std::strong_ordering::greater;
    }

    /* otherwise other has service areas and we only have external service areas, so we are lesser (just for ordering) */
    return std::strong_ordering::less;
}

std::string UniqueMbsSessionId::repr() const
{
    if (!m_mbsSessionId) return "UniqueMBSSessionId[null]";

    std::string result{"UniqueMBSSessionId["};
    result += MBSF_NAMESPACE_NAME(repr)(*m_mbsSessionId);
    if (m_mbsServiceArea) {
        result += ", " + MBSF_NAMESPACE_NAME(repr)(*m_mbsServiceArea);
    }
    if (m_extMBSServiceArea) {
        result += ", " + MBSF_NAMESPACE_NAME(repr)(*m_extMBSServiceArea);
    }
    result += "]";
    return result;
}

std::size_t UniqueMbsSessionId::hash() const
{
    std::size_t result{0x2f5982bf62a3607f}; // random number to initialise
    if (m_mbsSessionId) {
        std::hash<MbsSessionId> mbs_session_id_hash;
        result ^= mbs_session_id_hash(*m_mbsSessionId);
        if (m_mbsServiceArea) {
            std::hash<MbsServiceArea> mbs_service_area_hash;
            result ^= mbs_service_area_hash(*m_mbsServiceArea);
        }
        if (m_extMBSServiceArea) {
            std::hash<ExternalMbsServiceArea> ext_mbs_service_area_hash;
            result ^= ext_mbs_service_area_hash(*m_extMBSServiceArea);
        }
    }
    return result;
}

static bool ncgi_tai_overlap(const NcgiTai &a, const NcgiTai &b)
{
    if (*a.getTai() != *b.getTai()) return false;
    for (const auto &a_ncgi_item : a.getCellList()) {
        if (a_ncgi_item && &a_ncgi_item.value()) {
            const auto &a_ncgi = *a_ncgi_item.value();
            const auto &b_ncgi_list = b.getCellList();
            auto it = std::find_if(b_ncgi_list.begin(), b_ncgi_list.end(),
                                       [a_ncgi](const NcgiTai::CellListItemType &b_ncgi_item) -> bool {
                                           if (!b_ncgi_item || !b_ncgi_item.value()) return false;
                                           return a_ncgi == *b_ncgi_item.value();
                                       });
            if (it != b_ncgi_list.end()) return true;
        }
    }
    return false;
}

static bool svc_area_overlap(const std::shared_ptr<MbsServiceArea> &a, const std::shared_ptr<MbsServiceArea> &b)
{
    /* check for any overlap in the service areas */
    ogs_assert(a && b);

    const auto &a_ncgi_list = a->getNcgiList();
    const auto &b_ncgi_list = b->getNcgiList();
    const auto &a_tai_list = a->getTaiList();
    const auto &b_tai_list = b->getTaiList();


    if (a_ncgi_list && (b_ncgi_list || b_tai_list)) {
        for (const auto &a_ncgi_item : a_ncgi_list.value()) { /* a_ncgi_item is std::optional<std::shared_ptr<NcgiTai>> */
            if (a_ncgi_item && a_ncgi_item.value()) {
                const auto &a_ncgi_tai = *a_ncgi_item.value();
                if (b_ncgi_list) {
                    auto it = std::find_if(b_ncgi_list.value().begin(), b_ncgi_list.value().end(),
                                       [a_ncgi_tai](const MbsServiceArea::NcgiListItemType &b_ncgi_item) -> bool {
                                           if (!b_ncgi_item || !b_ncgi_item.value()) return false;
                                           return ncgi_tai_overlap(a_ncgi_tai, *b_ncgi_item.value());
                                       });
                    if (it != b_ncgi_list.value().end()) return true;
                }
                if (b_tai_list) {
                    auto it = std::find_if(b_tai_list.value().begin(), b_tai_list.value().end(),
                                        [a_ncgi_tai](const MbsServiceArea::TaiListItemType &b_tai_item) -> bool {
                                            if (!b_tai_item || !b_tai_item.value()) return false;
                                            return *a_ncgi_tai.getTai() == *b_tai_item.value();
                                        });
                    if (it != b_tai_list.value().end()) return true;
                }
            }
        }
    }

    if (a_tai_list && (b_ncgi_list || b_tai_list)) {
        for (const auto &a_tai_item : a_tai_list.value()) { /* a_tai_item is std::optional<std::shared_ptr<Tai>> */
            if (a_tai_item && a_tai_item.value()) {
                const auto &a_tai = *a_tai_item.value();
                if (b_tai_list) {
                    auto it = std::find_if(b_tai_list.value().begin(), b_tai_list.value().end(),
                                        [a_tai](const MbsServiceArea::TaiListItemType &b_tai_item) -> bool {
                                            return (b_tai_item && b_tai_item.value() && a_tai == *b_tai_item.value());
                                        });
                    if (it != b_tai_list.value().end()) return true;
                }
                if (b_ncgi_list) {
                    auto it = std::find_if(b_ncgi_list.value().begin(), b_ncgi_list.value().end(),
                                        [a_tai](const MbsServiceArea::NcgiListItemType &b_ncgi_item) -> bool {
                                            if (!b_ncgi_item || !b_ncgi_item.value()) return false;
                                            return a_tai == *b_ncgi_item.value()->getTai();
                                        });
                    if (it != b_ncgi_list.value().end()) return true;
                }
            }
        }
    }

    return false;
}

static bool ext_svc_area_overlap(const std::shared_ptr<ExternalMbsServiceArea> &a, const std::shared_ptr<ExternalMbsServiceArea> &b)
{
    /* check for any overlap in the service areas */
    ogs_assert(a && b);

    const auto &a_geog_list = a->getGeographicAreaList();
    const auto &b_geog_list = b->getGeographicAreaList();

    if (a_geog_list && b_geog_list) {
        for (const auto &a_geog_item : a_geog_list.value()) { /* a_geog_item is std::optional<std::shared_ptr<GeographicArea>> */
            if (a_geog_item && a_geog_item.value()) {
                const auto &a_geog_area = *a_geog_item.value();
                auto it = std::find_if(b_geog_list.value().begin(), b_geog_list.value().end(),
                                        [a_geog_area](const ExternalMbsServiceArea::GeographicAreaListItemType &b_geog_item) -> bool
                                        {
                                            return (b_geog_item && b_geog_item.value() && a_geog_area == *b_geog_item.value());
                                        });
                if (it != b_geog_list.value().end()) return true;
            }
        }
    }

    const auto &a_civic_list = a->getCivicAddressList();
    const auto &b_civic_list = b->getCivicAddressList();

    if (a_civic_list && b_civic_list) {
        for (const auto &a_civic_item : a_civic_list.value()) { /* a_civic_item is std::optional<std::shared_ptr<CivicAddress>> */
            if (a_civic_item && a_civic_item.value()) {
                const auto &a_civic_addr = *a_civic_item.value();
                auto it = std::find_if(b_civic_list.value().begin(), b_civic_list.value().end(),
                                        [a_civic_addr](const ExternalMbsServiceArea::CivicAddressListItemType &b_civic_item) -> bool
                                        {
                                            return (b_civic_item && b_civic_item.value() && a_civic_addr == *b_civic_item.value());
                                        });
                if (it != b_civic_list.value().end()) return true;
            }
        }
    }

    return false;
}

static std::strong_ordering mbs_session_ordering(bool a_tmgi_req, const std::shared_ptr<MbsSessionId> &a_mbs_session_id,
                                                 bool b_tmgi_req, const std::shared_ptr<MbsSessionId> &b_mbs_session_id)
{
    /* pointers already checked and not NULL */

    if (!a_tmgi_req && !b_tmgi_req) {
        auto a_tmgi = a_mbs_session_id->getTmgi();
        auto b_tmgi = b_mbs_session_id->getTmgi();
        if (a_tmgi && !b_tmgi) return std::strong_ordering::greater;
        if (!a_tmgi && b_tmgi) return std::strong_ordering::less;
        if (a_tmgi && *a_tmgi.value() != *b_tmgi.value()) return tmgi_ordering(*a_tmgi.value(), *b_tmgi.value());
    }

    return ssm_ordering(a_mbs_session_id->getSsm(), b_mbs_session_id->getSsm());
}


static std::strong_ordering svc_area_ordering(const std::shared_ptr<MbsServiceArea> &a, const std::shared_ptr<MbsServiceArea> &b)
{
    ogs_assert(a && b);

    const auto &a_ncgi_list = a->getNcgiList();
    const auto &b_ncgi_list = b->getNcgiList();

    if (!a_ncgi_list && b_ncgi_list) return std::strong_ordering::less;
    if (a_ncgi_list && !b_ncgi_list) return std::strong_ordering::greater;

    if (a_ncgi_list) {
        auto a_ncgi_ordered = a_ncgi_list.value();
        auto b_ncgi_ordered = b_ncgi_list.value();
        a_ncgi_ordered.sort(NcgiTaiLess());
        b_ncgi_ordered.sort(NcgiTaiLess());

        for (auto a_it = a_ncgi_ordered.begin(), b_it = b_ncgi_ordered.begin();
             a_it != a_ncgi_ordered.end() && b_it != b_ncgi_ordered.end(); ) {
            if (a_it == a_ncgi_ordered.end()) return std::strong_ordering::less;
            if (b_it == b_ncgi_ordered.end()) return std::strong_ordering::greater;
            if (*a_it != *b_it) {
                if (NcgiTaiLess()(*a_it, *b_it)) return std::strong_ordering::less;
                return std::strong_ordering::greater;
            }
            if (a_it != a_ncgi_ordered.end()) a_it++;
            if (b_it != b_ncgi_ordered.end()) b_it++;
        }
    }

    /* NcgiTai lists are equal or absent */

    const auto &a_tai_list = a->getTaiList();
    const auto &b_tai_list = b->getTaiList();

    if (!a_tai_list && b_tai_list) return std::strong_ordering::less;
    if (a_tai_list && !b_tai_list) return std::strong_ordering::greater;

    if (a_tai_list) {
        auto a_tai_ordered = a_tai_list.value();
        auto b_tai_ordered = b_tai_list.value();
        a_tai_ordered.sort(TaiLess());
        b_tai_ordered.sort(TaiLess());

        for (auto a_it = a_tai_ordered.begin(), b_it = b_tai_ordered.begin();
             a_it != a_tai_ordered.end() && b_it != b_tai_ordered.end(); ) {
            if (a_it == a_tai_ordered.end()) return std::strong_ordering::less;
            if (b_it == b_tai_ordered.end()) return std::strong_ordering::greater;
            if (*a_it != *b_it) {
                if (TaiLess()(*a_it, *b_it)) return std::strong_ordering::less;
                return std::strong_ordering::greater;
            }
            if (a_it != a_tai_ordered.end()) a_it++;
            if (b_it != b_tai_ordered.end()) b_it++;
        }
    }

    return std::strong_ordering::equal;
}

static std::strong_ordering ext_svc_area_ordering(const std::shared_ptr<ExternalMbsServiceArea> &a, const std::shared_ptr<ExternalMbsServiceArea> &b)
{
    ogs_assert(a && b);

    // TODO: fill this in to compare two geographic areas or civic addresses

    return std::strong_ordering::equal;
}

static std::strong_ordering ssm_ordering(const std::optional<std::shared_ptr<Ssm>> &a,
                                         const std::optional<std::shared_ptr<Ssm>> &b)
{
    if (!a && !b) return std::strong_ordering::equal;
    if (!a) return std::strong_ordering::less;
    if (!b) return std::strong_ordering::greater;

    /* compare a & b values */
    auto result = ip_addr_ordering(*a.value()->getDestIpAddr(), *b.value()->getDestIpAddr());
    if (result == std::strong_ordering::equal) {
        return ip_addr_ordering(*a.value()->getSourceIpAddr(), *b.value()->getSourceIpAddr());
    }
    return result;
}

static std::strong_ordering ip_addr_ordering(const IpAddr &a, const IpAddr &b)
{
    const auto &a_ipv4 = a.getIpv4Addr();
    const auto &b_ipv4 = b.getIpv4Addr();

    if (!a_ipv4 && b_ipv4) return std::strong_ordering::less;
    if (a_ipv4 && !b_ipv4) return std::strong_ordering::greater;
    if (a_ipv4 && a_ipv4.value() != b_ipv4.value()) return a_ipv4.value() <=> b_ipv4.value();

    const auto &a_ipv6 = a.getIpv6Addr();
    const auto &b_ipv6 = b.getIpv6Addr();

    if (!a_ipv6 && b_ipv6) return std::strong_ordering::less;
    if (a_ipv6 && !b_ipv6) return std::strong_ordering::greater;
    if (a_ipv6 && *a_ipv6.value() != *b_ipv6.value()) return *a_ipv6.value() <=> *b_ipv6.value();

    const auto &a_ipv6prefix = a.getIpv6Prefix();
    const auto &b_ipv6prefix = b.getIpv6Prefix();

    if (!a_ipv6prefix && b_ipv6prefix) return std::strong_ordering::less;
    if (a_ipv6prefix && !b_ipv6prefix) return std::strong_ordering::greater;
    if (a_ipv6prefix) return *a_ipv6prefix.value() <=> *b_ipv6prefix.value();

    return std::strong_ordering::equal;
}

static std::strong_ordering tmgi_ordering(const Tmgi &a, const Tmgi &b)
{
    const auto &a_svc_id = a.getMbsServiceId();
    const auto &b_svc_id = b.getMbsServiceId();
    auto result = a_svc_id <=> b_svc_id;
    if (result != std::strong_ordering::equal) return result;

    auto a_plmn_id = a.getPlmnId();
    auto b_plmn_id = b.getPlmnId();
    if (*a_plmn_id != *b_plmn_id) {
        if (PlmnIdLess()(*a_plmn_id, *b_plmn_id)) return std::strong_ordering::less;
        return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
