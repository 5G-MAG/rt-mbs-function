/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MultipartMime class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
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

#include <filesystem>
#include <format>
#include <list>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "common.hh"

#include "MultipartMime.hh"
#include <DocrootFile.hh>

namespace std {
    template<>
    struct formatter<MBSF_NAMESPACE_NAME(MultipartMime::MultipartType), char> {
        template <class ParseContext>
        constexpr ParseContext::iterator parse(ParseContext& ctx) {
            return ctx.begin();
        };
        template <class FmtContext>
        FmtContext::iterator format(MBSF_NAMESPACE_NAME(MultipartMime::MultipartType) multipart_type, FmtContext& ctx) const {
            std::string typ_name;
            switch (multipart_type) {
            case MBSF_NAMESPACE_NAME(MultipartMime::MIXED):
                typ_name = "mixed";
                break;
            case MBSF_NAMESPACE_NAME(MultipartMime::RELATED):
                typ_name = "related";
                break;
            case MBSF_NAMESPACE_NAME(MultipartMime::ALTERNATIVE):
                typ_name = "alternative";
                break;
            default:
                break;
            }
            return std::format_to(ctx.out(), "{}", typ_name);
        };
    };
}

HTTPXPP_NAMESPACE_USING(DocrootFile);

MBSF_NAMESPACE_START

static std::string random_string(size_t chars);

MultipartMime::MultipartMime(MultipartMime::MultipartType typ)
    :m_headers()
    ,m_body()
    ,m_separator(random_string(64))
    ,m_bodyFooterSepPos(m_body.end())
{
    m_headers.insert(std::make_pair(std::string{"Content-Type"}, std::format("multipart/{}; boundary=\"{}\"", typ, m_separator)));
    __insertFooterSep();
}

void MultipartMime::addFile(const std::filesystem::path &rootdir, const std::filesystem::path &filename, const std::optional<std::string> &disposition_type)
{
    __removeFooterSep();
    __insertSep();

    DocrootFile infile(rootdir / filename);
    for (const auto &[field, value] : infile.headers()) {
        auto header_line = std::format("{}: {}\r\n", field, value);
        m_body.insert(m_body.end(), header_line.begin(), header_line.end());
    }

    if (disposition_type) {
        std::string disposition_hdr = std::format("Content-Disposition: {}; filename=\"{}\"\r\n", disposition_type.value(), filename.filename().string());
        m_body.insert(m_body.end(), disposition_hdr.begin(), disposition_hdr.end());
    }
    std::string location_hdr = std::format("Content-Location: \"{}\"\r\n", filename.string());
    m_body.insert(m_body.end(), location_hdr.begin(), location_hdr.end());

    static const std::string crlf{"\r\n"};
    m_body.insert(m_body.end(), crlf.begin(), crlf.end());

    m_body.insert(m_body.end(), infile.body().begin(), infile.body().end());
    m_body.insert(m_body.end(), crlf.begin(), crlf.end());

    __insertFooterSep();
}

void MultipartMime::__insertSep() {
    auto sep = std::format("--{}\r\n", m_separator);
    m_body.insert(m_body.end(), sep.begin(), sep.end());
}

void MultipartMime::__insertFooterSep() {
    auto footer_sep = std::format("--{}--\r\n", m_separator);
    m_bodyFooterSepPos = m_body.insert(m_body.end(), footer_sep.begin(), footer_sep.end());
}

void MultipartMime::__removeFooterSep() {
    m_body.erase(m_bodyFooterSepPos, m_body.end());
}

static std::string random_string(size_t chars)
{
    std::string result;
    static const char base_charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@/?\\+=_!:;";

    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(0, sizeof(base_charset)-2);
    for (size_t i = 0; i < chars; i++) {
        result += base_charset[uniform_dist(e1)];
    }

    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
