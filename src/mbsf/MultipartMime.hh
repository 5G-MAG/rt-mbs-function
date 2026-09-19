#ifndef _MBSF_MULTIPART_MIME_HH_
#define _MBSF_MULTIPART_MIME_HH_
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
#include <optional>
#include <string>
#include <vector>

#include "common.hh"
#include <DocrootHTTPRequestHandler.hh>

MBSF_NAMESPACE_START

class MultipartMime {
public:
    enum MultipartType {
        MIXED,
        RELATED,
        ALTERNATIVE
    };

    MultipartMime(MultipartType typ = MIXED);

    MultipartMime(const MultipartMime &other) = delete;
    MultipartMime(MultipartMime &&other) = delete;

    virtual ~MultipartMime() {};

    MultipartMime &operator=(const MultipartMime &other) = delete;
    MultipartMime &operator=(MultipartMime &&other) = delete;

    void addFile(const std::filesystem::path &rootdir, const std::filesystem::path &filename, const std::optional<std::string> &disposition_type = std::nullopt);

    // Lower-level primitive addFile() itself now delegates to: adds one part built entirely
    // in memory (e.g. a freshly-serialised JSON document, RFC 2387's own root body part, which
    // has no file on disk to read via addFile()/DocrootFile). content_location, if given, is a
    // bare URI value per RFC 2557 clause 4.1 (never a quoted string) -- see addFile()'s own
    // comment on this for why; pass std::nullopt for a part that needs no Content-Location
    // (the root part, per RFC 2387, does not need one -- it is the default "start" part in the
    // absence of the Content-Type "start" parameter this class does not set).
    void addPart(const std::vector<char> &body, const std::map<std::string, std::string> &headers,
                 const std::optional<std::string> &disposition_type = std::nullopt,
                 const std::optional<std::string> &content_location = std::nullopt);

    const std::vector<char> &body() const { return m_body; };
    const std::map<std::string, std::string> &headers() const { return m_headers; };

private:
    void __insertSep();
    void __insertFooterSep();
    void __removeFooterSep();

    std::map<std::string, std::string> m_headers;
    std::vector<char> m_body;
    std::string m_separator;
    std::vector<char>::iterator m_bodyFooterSepPos;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_MULTIPART_MIME_HH_ */
