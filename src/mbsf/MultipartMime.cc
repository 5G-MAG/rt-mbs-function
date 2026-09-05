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
static std::string content_location_value(const std::string &uri);
static std::string encode_atom(const std::string &raw_str);
static std::string encode_mime_token(const std::string &raw_str);
static std::string escape_chars(const std::string_view &s, char esc, const std::string &other_chars_to_esc);

MultipartMime::MultipartMime(MultipartMime::MultipartType typ)
    :m_headers()
    ,m_body()
    ,m_separator(random_string(64))
    ,m_bodyFooterSepPos(m_body.end())
{
    m_headers.insert(std::make_pair(std::string{"Content-Type"}, std::format("multipart/{}; boundary={}", typ, encode_mime_token(m_separator))));
    __insertFooterSep();
}

void MultipartMime::addFile(const std::filesystem::path &rootdir, const std::filesystem::path &filename, const std::optional<std::string> &disposition_type)
{
    DocrootFile infile(rootdir / filename);
    addPart(infile.body(), infile.headers(), disposition_type, filename.string());
}

void MultipartMime::addPart(const std::vector<char> &body, const std::map<std::string, std::string> &headers,
                             const std::optional<std::string> &disposition_type,
                             const std::optional<std::string> &content_location)
{
    __removeFooterSep();
    __insertSep();

    for (const auto &[field, value] : headers) {
        auto header_line = std::format("{}: {}\r\n", field, value);
        m_body.insert(m_body.end(), header_line.begin(), header_line.end());
    }

    if (disposition_type) {
        std::string filename_part = content_location.value_or(std::string());
        std::string disposition_hdr = std::format("Content-Disposition: {}; filename={}\r\n", disposition_type.value(), encode_mime_token(filename_part));
        m_body.insert(m_body.end(), disposition_hdr.begin(), disposition_hdr.end());
    }
    if (content_location) {
        /* The value is the bare URI, never a quoted string.
           RFC 2557 clause 4.1: "content-location = "Content-Location:" [CFWS] URI [CFWS]", with
           "URI = absoluteURI | relativeURI". CFWS admits comments and folding white space only, so
           there is no production in which a DQUOTE can appear; a receiver reading a quoted value
           either fails to parse the header or derives a URI that includes the quote characters, and
           the cross-reference in TS 26.517 V18.6.0 clause 5.3.1A then never matches.

           Finding F11: clause 4.4.1 continues -- "If such a URI occurs, all spaces and other illegal
           characters in it must be encoded using one of the methods described in [MIME3] section 4"
           -- [MIME3] is RFC 2047, whose section 4 defines the "=?charset?encoding?encoded-text?="
           encoded-word syntax. Only applied when the value actually needs it, since clause 4.4.1's
           own "if such a URI occurs" is conditional -- an ordinary path is left as a bare URI exactly
           as the paragraph above requires. */
        std::string location_hdr = std::format("Content-Location: {}\r\n", content_location_value(*content_location));
        m_body.insert(m_body.end(), location_hdr.begin(), location_hdr.end());
    }

    static const std::string crlf{"\r\n"};
    m_body.insert(m_body.end(), crlf.begin(), crlf.end());

    m_body.insert(m_body.end(), body.begin(), body.end());
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


// RFC 2557 clause 4.4.1: "Some documents may contain URIs with characters that are inappropriate
// for an RFC 822 header... If such a URI occurs, all spaces and other illegal characters in it
// must be encoded". A plain RFC 822/5322 unstructured header field body permits printable
// US-ASCII and folding whitespace only -- control characters, DEL and any non-ASCII octet are
// illegal there regardless of this clause, and clause 4.4.1 additionally names space itself
// (otherwise a legal header character) as requiring encoding here, since Content-Location's value
// must be recovered exactly, with no whitespace folding/normalisation ambiguity.
static bool content_location_needs_encoding(const std::string &uri)
{
    for (unsigned char c : uri) {
        if (c == ' ' || c < 0x20 || c >= 0x7f) return true;
    }
    return false;
}

// RFC 2557 clause 4.4.1: "...must be encoded using one of the methods described in [MIME3]
// section 4" -- [MIME3] is RFC 2047 ("MIME Part Three: Message Header Extensions for Non-ASCII
// Text"), whose section 4 defines encoded-word syntax "=?charset?encoding?encoded-text?=" with
// two encodings, "Q" and "B"; section 4 itself: "The 'Q' encoding is recommended for use when
// most of the characters to be encoded are in the ASCII character set" -- true for a docroot
// path, which is what this function encodes. Section 4.2's Q-encoding rules: any octet may be
// represented as "=" followed by its two hex digits (rule 1); hex 20 (space) may be represented
// as "_" instead (rule 2); any other printable ASCII character except "=", "?" and "_" (reserved
// by the encoded-word syntax itself) may appear literally (rule 3).
//
// Clause 4.4.1 also: "The charset parameter value 'US-ASCII' SHOULD be used if the URI contains
// no octets outside of the 7-bit range... If [a correct charset] cannot be safely established,
// the value 'UNKNOWN-8BIT' [RFC 1428] MUST be used." This MBSF has no metadata about what
// encoding an operator's docroot filenames are actually in, so a non-ASCII octet here always
// falls into the "cannot be safely established" case rather than assuming e.g. UTF-8.
static std::string content_location_encode(const std::string &uri)
{
    bool ascii_only = true;
    for (unsigned char c : uri) {
        if (c >= 0x7f) {
            ascii_only = false;
            break;
        }
    }
    const char *charset = ascii_only ? "US-ASCII" : "UNKNOWN-8BIT";

    std::string encoded_text;
    encoded_text.reserve(uri.size());
    for (unsigned char c : uri) {
        if (c == ' ') {
            encoded_text.push_back('_');
        } else if (c > 0x20 && c < 0x7f && c != '=' && c != '?' && c != '_') {
            encoded_text.push_back(static_cast<char>(c));
        } else {
            encoded_text += std::format("={:02X}", c);
        }
    }
    return std::format("=?{}?Q?{}?=", charset, encoded_text);
}

static std::string content_location_value(const std::string &uri)
{
    if (!content_location_needs_encoding(uri)) return uri;
    return content_location_encode(uri);
}

static std::string random_string(size_t chars)
{
    std::string result;
    // RFC 2046 SS5.1.1's boundary grammar only allows bcharsnospace (DIGIT / ALPHA / "'" / "("
    // / ")" / "+" / "_" / "," / "-" / "." / "/" / ":" / "=" / "?"), quoted or not -- '@', '\'
    // and '!' were never legal boundary characters, and ';' isn't either. '\' was additionally
    // being written unescaped into the quoted-string Content-Type header parameter, so a
    // strict RFC 2045 quoted-string parser would derive a different (unescaped) boundary value
    // than the literal, un-unescaped delimiter text actually used in the body.
    //
    // Per review feedback (PR #42): the previous charset dropped some *legal* bcharsnospace
    // characters ("'", "(", ")", ",", "-", ".") along with the illegal ones -- restore the full
    // legal set.
    static const char base_charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'()+_,-./:=?";

    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(0, sizeof(base_charset)-2);
    for (size_t i = 0; i < chars; i++) {
        result += base_charset[uniform_dist(e1)];
    }

    return result;
}

static std::string encode_value(const std::string &raw_str, const char *unquoted_chars)
{
    static const char q_esc = '\\';
    static const std::string q_other_esc("\"");
    if (raw_str.find_first_not_of(unquoted_chars) != std::string::npos) {
        // string contains special chars, quote it
        return std::format("\"{}\"", escape_chars(raw_str, q_esc, q_other_esc));
    }
    return raw_str;
}

// RFC 2822 Section 3.2.4 atext -- valid unquoted characters for a "word" (Content-Location is
// defined via RFC 822's word/atom grammar per RFC 2557 Section 4.1, not a MIME parameter).
static std::string encode_atom(const std::string &raw_str)
{
    static const char unquoted_atom_chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-/=?^_`{|}~";
    return encode_value(raw_str, unquoted_atom_chars);
}

// RFC 2045 Section 5.1 token -- valid unquoted characters for a MIME parameter value
// (Content-Type's boundary=, Content-Disposition's filename=). Excludes tspecials
// ( ) < > @ , ; : \ " / [ ] ? = -- notably '/', ':', '=', '?', all of which random_string()'s
// own boundary charset can legitimately produce -- and additionally permits '.', which atext
// does not.
static std::string encode_mime_token(const std::string &raw_str)
{
    static const char unquoted_token_chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`{|}~";
    return encode_value(raw_str, unquoted_token_chars);
}

static std::string escape_chars(const std::string_view &s, char esc, const std::string &other_esc_chars)
{
    std::string result;
    for (char c : s) {
        if (c == esc || other_esc_chars.find_first_of(c) != std::string::npos) {
            result += esc;
        }
        result += c;
    }
    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
