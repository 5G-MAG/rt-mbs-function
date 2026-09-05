/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: MultipartMime unit test
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

/* Covers MultipartMime's own body-assembly logic directly -- no App::self() dependency anywhere
 * in this class or in DocrootFile, so unlike test_announcement_bundle_gating.cc (abandoned, see
 * rt-mbs-function.md's own register entry), this one genuinely link-isolates. */

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "MultipartMime.hh"

using MBSF_NAMESPACE::MultipartMime;

static size_t total = 0, failed = 0;

#define CHECK(cond, msg) do { \
    total++; \
    if (!(cond)) { failed++; std::fprintf(stderr, "FAILED: %s\n", msg); } \
    else { std::fprintf(stderr, "OK: %s\n", msg); } \
} while (0)

static std::string body_str(const MultipartMime &m)
{
    return std::string(m.body().begin(), m.body().end());
}

static std::string boundary_of(const MultipartMime &m)
{
    // Content-Type: multipart/<type>; boundary="<sep>" -- extract <sep>.
    const auto &headers = m.headers();
    auto it = headers.find("Content-Type");
    if (it == headers.end()) return {};
    const std::string &ct = it->second;
    auto q1 = ct.find('"');
    if (q1 == std::string::npos) return {};
    auto q2 = ct.find('"', q1 + 1);
    if (q2 == std::string::npos) return {};
    return ct.substr(q1 + 1, q2 - q1 - 1);
}

int main()
{
    // Case 1: a freshly-constructed, empty MultipartMime already carries a well-formed
    // Content-Type header and its own (empty) closing boundary -- RFC 2046 clause 5.1.1's own
    // grammar requires a body to end "--boundary--", even with zero parts.
    {
        MultipartMime m(MultipartMime::RELATED);
        CHECK(m.headers().at("Content-Type").starts_with("multipart/related; boundary=\""),
              "constructor sets a multipart/related Content-Type header with a boundary parameter");
        std::string boundary = boundary_of(m);
        CHECK(!boundary.empty(), "the boundary value is non-empty");
        std::string closing = "--" + boundary + "--\r\n";
        CHECK(body_str(m) == closing, "an empty MultipartMime's body is exactly its own closing boundary");
    }

    // Case 2: a single addPart() -- RFC 2387 clause 3.1's own default-root rule: "In the absence
    // of the start parameter, ... the first body part" is the root -- this only matters once a
    // second part exists (case 3), but the single-part shape must already be right: opening
    // boundary, headers, blank line, body, then the closing boundary, in that order.
    {
        MultipartMime m(MultipartMime::MIXED);
        std::string boundary = boundary_of(m);
        std::vector<char> content{'h', 'e', 'l', 'l', 'o'};
        m.addPart(content, {{"Content-Type", "text/plain"}});
        std::string body = body_str(m);

        std::string expected =
            "--" + boundary + "\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "hello\r\n"
            "--" + boundary + "--\r\n";
        CHECK(body == expected, "a single addPart() produces exactly one opening boundary, header, blank line, body, closing boundary");
    }

    // Case 3: RFC 2387 clause 3.1's own default-root rule, the exact bug this class's own
    // introducing commit fixed in UserServiceDiscoveryHandler -- confirms the ordering the fix
    // depends on is a property of MultipartMime itself, not just how the caller happened to use it.
    {
        MultipartMime m(MultipartMime::RELATED);
        std::vector<char> root_content{'r', 'o', 'o', 't'};
        std::vector<char> dep_content{'d', 'e', 'p'};
        m.addPart(root_content, {{"Content-Type", "application/json"}});
        m.addPart(dep_content, {{"Content-Type", "application/sdp"}}, std::nullopt, "session.sdp");
        std::string body = body_str(m);

        auto root_pos = body.find("root");
        auto dep_pos = body.find("dep");
        CHECK(root_pos != std::string::npos && dep_pos != std::string::npos && root_pos < dep_pos,
              "the first-added part appears before the second in the assembled body (RFC 2387 default-root ordering)");

        // Exactly one closing boundary, and it is the very last thing in the body -- a second,
        // stray closing boundary left behind from the first addPart() would mean __removeFooterSep()
        // failed to do its job before the second part was appended.
        std::string boundary = boundary_of(m);
        std::string closing = "--" + boundary + "--\r\n";
        size_t closing_count = 0;
        for (size_t pos = 0; (pos = body.find(closing, pos)) != std::string::npos; pos += closing.size()) closing_count++;
        CHECK(closing_count == 1, "exactly one closing boundary exists after two addPart() calls");
        CHECK(body.ends_with(closing), "the closing boundary is the last thing in the body");
    }

    // Case 4: Content-Disposition filename quoting -- RFC 822 SS3.3's quoted-string grammar
    // (referenced by RFC 2045 for parameter values) requires a literal backslash or double-quote
    // inside the value to be escaped with a preceding backslash.
    {
        MultipartMime m(MultipartMime::MIXED);
        std::vector<char> content{'x'};
        m.addPart(content, {}, std::string("attachment"), "a\"b\\c");
        std::string body = body_str(m);
        CHECK(body.find("filename=\"a\\\"b\\\\c\"") != std::string::npos,
              "a filename containing a quote and a backslash is escaped per RFC 822 quoted-string rules");
    }

    // Case 5: Content-Location -- RFC 2557 clause 4.1's own grammar has no production admitting a
    // DQUOTE, so the value must be a bare URI, never quoted; an ASCII path with no space or
    // control character needs no RFC 2047 encoding (clause 4.4.1's own "if such a URI occurs" is
    // conditional).
    {
        MultipartMime m(MultipartMime::RELATED);
        std::vector<char> content{'x'};
        m.addPart(content, {}, std::nullopt, "plain/path.sdp");
        std::string body = body_str(m);
        CHECK(body.find("Content-Location: plain/path.sdp\r\n") != std::string::npos,
              "an ASCII, space-free Content-Location value is emitted bare, unquoted, unencoded");
    }

    // Case 6: Content-Location needing RFC 2047 encoding -- a space is itself illegal in a plain
    // RFC 5322 unstructured header value and clause 4.4.1 names it explicitly as requiring
    // encoding here (recovered exactly, no whitespace-folding ambiguity).
    {
        MultipartMime m(MultipartMime::RELATED);
        std::vector<char> content{'x'};
        m.addPart(content, {}, std::nullopt, "a path.sdp");
        std::string body = body_str(m);
        CHECK(body.find("Content-Location: =?US-ASCII?Q?a_path.sdp?=\r\n") != std::string::npos,
              "a Content-Location value containing a space is Q-encoded per RFC 2047 section 4, space as '_'");
    }

    // Case 7: addFile() -- reads a real file plus its DocrootFile-derived Content-Type, and its
    // body/headers reach addPart() exactly as a directly-supplied in-memory part would.
    {
        std::filesystem::path dir = std::filesystem::temp_directory_path() / "mbsf_multipart_test";
        std::filesystem::create_directories(dir);
        std::filesystem::path filename = "test.json";
        {
            std::ofstream f(dir / filename, std::ios::binary);
            f << "{\"a\":1}";
        }
        MultipartMime m(MultipartMime::RELATED);
        m.addFile(dir, filename);
        std::string body = body_str(m);
        CHECK(body.find("{\"a\":1}") != std::string::npos, "addFile() includes the real file's own content in the assembled body");
        CHECK(body.find("Content-Location: test.json\r\n") != std::string::npos,
              "addFile() sets Content-Location to the filename passed in, by default");
        std::filesystem::remove_all(dir);
    }

    std::fprintf(stderr, "%zu/%zu checks passed\n", total - failed, total);
    return failed == 0 ? 0 : 1;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
