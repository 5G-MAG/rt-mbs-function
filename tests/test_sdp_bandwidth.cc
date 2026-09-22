/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: SDP bandwidth transport-overhead unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

/* Covers UserServiceAnnBundle::sdpBandwidthBitRate(), the rule that turns the provisioned content
 * bit rate into the value written to the session description's bandwidth line.
 *
 * TS 26.346 V18.2.0 clause 7.3.2.10 requires that value to count whole packets, transport headers
 * included. The rate the MBSF holds paces the ALC bytes only, so the conversion adds the IP and UDP
 * headers each packet carries. The MTU those are measured against is operator-configured, because
 * neither Nmb10 nor Nmb9 carries it and no clause supplies a default.
 */

#include "UserServiceAnnBundle.hh"

#include <cstdio>
#include <cstdlib>
#include <optional>

using namespace com::fiveg_mag::ref_tools::mbsf;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) {
        g_pass++;
        printf("INFO: %s passed.\n", what);
    } else {
        g_fail++;
        printf("FAIL: %s\n", what);
    }
}

int main(void)
{
    const uint64_t rate = 5000000; /* 5 Mbps, the figure TS 26.502's own example uses for video */

    /* With no MTU configured the rate is written through untouched, which is what happened before
       the option existed. The clause is then not satisfied, and the caller logs that. */
    check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, std::nullopt, false) == rate,
          "an unconfigured MTU leaves the bit rate unchanged");

    /* IPv4: 20 octets of IP plus 8 of UDP against a 1500-byte MTU, so 1472 bytes of ALC per packet
       and the announced rate is 1500/1472 of the paced one. */
    {
        const uint64_t expect = static_cast<uint64_t>(static_cast<double>(rate) * 1500 / (1500 - 28));
        check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 1500, false) == expect,
              "IPv4 adds the 28-byte IP and UDP header per packet");
        check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 1500, false) > rate,
              "the IPv4 announced rate exceeds the paced rate");
    }

    /* IPv6: 40 octets of IP plus 8 of UDP, so a larger adjustment than IPv4 at the same MTU. */
    {
        const uint64_t expect = static_cast<uint64_t>(static_cast<double>(rate) * 1500 / (1500 - 48));
        check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 1500, true) == expect,
              "IPv6 adds the 48-byte IP and UDP header per packet");
        check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 1500, true) >
              UserServiceAnnBundle::sdpBandwidthBitRate(rate, 1500, false),
              "IPv6 is adjusted by more than IPv4 at the same MTU");
    }

    /* A jumbo frame carries proportionally less header, so the adjustment shrinks. */
    check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 9000, false) <
          UserServiceAnnBundle::sdpBandwidthBitRate(rate, 1500, false),
          "a larger MTU needs a smaller adjustment");

    /* An MTU that cannot hold the transport header is a misconfiguration. Returning the rate
       unchanged keeps the caller's own error path reachable and never divides by zero. */
    check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 28, false) == rate,
          "an MTU equal to the IPv4 transport header is refused");
    check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 20, false) == rate,
          "an MTU below the IPv4 transport header is refused");
    check(UserServiceAnnBundle::sdpBandwidthBitRate(rate, 48, true) == rate,
          "an MTU equal to the IPv6 transport header is refused");

    /* A zero rate stays zero rather than becoming a small positive number. */
    check(UserServiceAnnBundle::sdpBandwidthBitRate(0, 1500, false) == 0,
          "a zero bit rate is unaffected by the adjustment");

    printf("Test: SDP bandwidth Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
