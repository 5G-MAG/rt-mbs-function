/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: multiplex signature unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 ******************************************************************************/

/* Covers multiplexed() and multiplex_signature(). The members of a multiplex (TS 29.580 V18.8.0 table 6.2.6.2.4-1,
 * multiplexedServFlag) are put on one MB-SMF MBS Session, which is created with one set of target service areas and one
 * MBS Service Information, so they are compared on what the MB-SMF is told: members that say the same share the session,
 * members that say something else do not.
 */

#include "MultiplexSignature.hh"

#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/MbsServiceInfo.h"

#include <cstdio>
#include <memory>
#include <string>

using namespace fiveg_mag_reftools;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsServiceInfo;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

static std::shared_ptr<MbsServiceArea> area(const char *tac)
{
    CJson json = CJson::parse(std::string("{\"taiList\":[{\"plmnId\":{\"mcc\":\"001\",\"mnc\":\"01\"},\"tac\":\"") + tac + "\"}]}");
    return std::make_shared<MbsServiceArea>(json, true);
}

static std::shared_ptr<MbsServiceInfo> qos(const char *ambr)
{
    auto info = std::make_shared<MbsServiceInfo>();
    info->setMbsSessionAmbr(std::string(ambr));
    return info;
}

static std::shared_ptr<MBSDistributionSessionInfo> member()
{
    return std::make_shared<MBSDistributionSessionInfo>();
}

int main()
{
    {   /* the flag */
        auto m = member();
        check(!multiplexed(m), "no flag: not a member of a multiplex");
        m->setMultiplexedServFlag(false);
        check(!multiplexed(m), "flag false: not a member");
        m->setMultiplexedServFlag(true);
        check(multiplexed(m), "flag true: a member");
    }
    {   /* the same */
        auto a = member(), b = member();
        check(multiplex_signature(a) == multiplex_signature(b), "two empty members say the same");
        a->setTgtServAreas(area("000001")); b->setTgtServAreas(area("000001"));
        a->setMbsServInfo(qos("10 Mbps")); b->setMbsServInfo(qos("10 Mbps"));
        a->setMbsFSAId(std::string("00B001")); b->setMbsFSAId(std::string("00B001"));
        a->setLocationDependent(true); b->setLocationDependent(true);
        a->setRestrictedFlag(false); b->setRestrictedFlag(false);
        check(multiplex_signature(a) == multiplex_signature(b), "the same areas, QoS, FSA, location and restriction say the same");
        a->setMultiplexedServFlag(true);
        check(multiplex_signature(a) == multiplex_signature(b), "the flag itself is not part of what is compared");
    }
    {   /* different */
        auto base = [] {
            auto m = member();
            m->setTgtServAreas(area("000001"));
            m->setMbsServInfo(qos("10 Mbps"));
            m->setMbsFSAId(std::string("00B001"));
            return m;
        };
        auto a = base();
        { auto b = base(); b->setTgtServAreas(area("000002")); check(multiplex_signature(a) != multiplex_signature(b), "another target service area differs"); }
        { auto b = base(); b->setMbsServInfo(qos("20 Mbps")); check(multiplex_signature(a) != multiplex_signature(b), "another QoS differs"); }
        { auto b = base(); b->setMbsFSAId(std::string("00B002")); check(multiplex_signature(a) != multiplex_signature(b), "another FSA id differs"); }
        { auto b = base(); b->setLocationDependent(true); check(multiplex_signature(a) != multiplex_signature(b), "location dependence differs"); }
        { auto b = base(); b->setRestrictedFlag(true); check(multiplex_signature(a) != multiplex_signature(b), "restriction differs"); }
        { auto b = base(); b->setMbsServInfo(std::nullopt); check(multiplex_signature(a) != multiplex_signature(b), "a missing QoS differs from one given"); }
        { auto b = base(); b->setLocationDependent(false); check(multiplex_signature(a) == multiplex_signature(b), "location dependence false is the same as absent"); }
    }

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
