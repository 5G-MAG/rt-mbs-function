/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: announced session attribute set unit test
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

/* Covers UserServiceAnnBundle::announcedAttributes(), which decides whether a Distribution
 * Session's description carries a=flute-tsi and the FEC attributes.
 *
 * All four were emitted unconditionally, which review on 5G-MAG/rt-mbs-function#52 reported: a
 * Packet Distribution Session was announced with a FLUTE TSI it has no FLUTE session for, and every
 * session was announced with "a=FEC-declaration:0 encoding-id=0" and "a=FEC:0" whether or not it
 * carried FEC.
 *
 * TS 26.346 V18.2.0 clause 7.3.2.4 scopes the TSI descriptor to "a complete FLUTE SDP session
 * description". Clause 7.3.2.8 makes the FEC declaration optional and gives its absence a meaning,
 * and describes a=FEC as a reference to a declaration.
 */

#include "UserServiceAnnBundle.hh"

#include <cstdio>

using namespace com::fiveg_mag::ref_tools::mbsf;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; printf("INFO: %s passed.\n", what); }
    else    { g_fail++; printf("FAIL: %s\n", what); }
}

int main(void)
{
    /* Object Distribution runs FLUTE, so it is the only method with a TSI to declare. */
    const auto object_no_fec = UserServiceAnnBundle::announcedAttributes(true, false);
    check(object_no_fec.fluteTsi, "an Object Distribution Session announces a=flute-tsi");
    check(!object_no_fec.fec, "an Object Distribution Session with no FEC does not announce a=FEC");
    check(!object_no_fec.fecDeclaration,
          "an Object Distribution Session with no FEC does not announce a=FEC-declaration");
    check(!object_no_fec.fecRedundancyLevel,
          "an Object Distribution Session with no FEC does not announce a=FEC-redundancy-level");

    const auto object_fec = UserServiceAnnBundle::announcedAttributes(true, true);
    check(object_fec.fluteTsi, "an Object Distribution Session with FEC still announces a=flute-tsi");
    check(object_fec.fec && object_fec.fecDeclaration && object_fec.fecRedundancyLevel,
          "an Object Distribution Session with FEC announces all three FEC attributes");

    /* A Packet Distribution Session runs no FLUTE session. This is the case that was wrong. */
    const auto packet_no_fec = UserServiceAnnBundle::announcedAttributes(false, false);
    check(!packet_no_fec.fluteTsi, "a Packet Distribution Session does not announce a=flute-tsi");
    check(!packet_no_fec.fec && !packet_no_fec.fecDeclaration && !packet_no_fec.fecRedundancyLevel,
          "a Packet Distribution Session with no FEC announces no FEC attributes");

    /* A packet stream is FEC protected only if the MBSTF protects it, which it does not yet: announcing
       FEC for it would claim a protection the stream does not have. */
    const auto packet_fec = UserServiceAnnBundle::announcedAttributes(false, true);
    check(!packet_fec.fluteTsi,
          "a Packet Distribution Session with FEC still does not announce a=flute-tsi");
    check(UserServiceAnnBundle::packetFecSupported ||
              (!packet_fec.fec && !packet_fec.fecDeclaration && !packet_fec.fecRedundancyLevel),
          "a Packet Distribution Session announces no FEC attributes while the MBSTF applies no packet FEC");

    /* a=FEC references a declaration, so the two are never announced apart. */
    bool reference_always_has_declaration = true;
    for (int od = 0; od < 2; od++) {
        for (int fec = 0; fec < 2; fec++) {
            const auto a = UserServiceAnnBundle::announcedAttributes(od != 0, fec != 0);
            if (a.fec != a.fecDeclaration) reference_always_has_declaration = false;
        }
    }
    check(reference_always_has_declaration,
          "a=FEC is never announced without the a=FEC-declaration it references");

    /* The media line of a Packet Distribution Session comes from the Codec-Data of TS 29.214 clause 5.3.7. */
    {
        const auto rtp = UserServiceAnnBundle::packetMediaFromCodecData(
            {"downlink\noffer\nm=video 4002 RTP/AVP 96\na=rtpmap:96 H264/90000\n"
             "a=fmtp:96 profile-level-id=42A01E; packetization-mode=1\na=recvonly\nb=AS:77\n"});
        check(rtp && rtp->media == "video" && rtp->proto == "RTP/AVP" && rtp->fmt == "96",
              "an RTP codec description gives its media, protocol and payload type");
        check(rtp && rtp->attributes.size() == 2 && rtp->attributes[0].first == "rtpmap" &&
                  rtp->attributes[0].second == "96 H264/90000" && rtp->attributes[1].first == "fmtp",
              "its rtpmap and fmtp lines are kept, its recvonly and b= lines are not");

        const auto uplink_and_downlink = UserServiceAnnBundle::packetMediaFromCodecData(
            {"uplink\nanswer\nm=audio 5000 RTP/AVP 97\n", "downlink\ndescription\nm=video 4002 RTP/AVP 96\n"});
        check(uplink_and_downlink && uplink_and_downlink->media == "video",
              "a downlink entry is preferred over an uplink one");

        const auto crlf = UserServiceAnnBundle::packetMediaFromCodecData(
            {"downlink\r\noffer\r\nm=audio 4004 RTP/AVP 98 99\r\na=rtpmap:98 AMR/8000\r\n"});
        check(crlf && crlf->fmt == "98 99" && crlf->attributes.size() == 1,
              "CRLF line ends and several payload types are read");

        check(!UserServiceAnnBundle::packetMediaFromCodecData({}), "no codec entry gives no media line");
        check(!UserServiceAnnBundle::packetMediaFromCodecData({"downlink\noffer\n"}),
              "a codec entry without an m line gives no media line");
        check(!UserServiceAnnBundle::packetMediaFromCodecData({"downlink\noffer\na=rtpmap:96 H264/90000\n"}),
              "a codec entry whose first SDP line is not an m line gives no media line");
        check(!UserServiceAnnBundle::packetMediaFromCodecData({"downlink\noffer\nm=video 4002\n"}),
              "a truncated m line gives no media line");
    }

    printf("Test: SdpAnnouncedAttributes Pass: %d Fail: %d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
