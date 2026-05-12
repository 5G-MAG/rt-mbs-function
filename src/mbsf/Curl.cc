/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: cURL requests
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 *            David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <chrono>
#include <iostream>
#include <map>

#include "ogs-app.h"
#include "ogs-sbi.h" // include before "common.hh" to ensure correct logging domain

#include "common.hh"
#include "mbsf-version.h"
#include "utilities.hh"

#include "Curl.hh"

MBSF_NAMESPACE_START

class CurlGlobalCleanup {
public:
    CurlGlobalCleanup() {};
    ~CurlGlobalCleanup() {
        curl_global_cleanup();
    };
};

// Ensure cURL clean up called on program exit
static CurlGlobalCleanup g_curl_global_cleanup;

enum HeaderProcessingState {
    HEADER_START,
    HEADER_HEADERS,
    HEADER_BODY,
    HEADER_TRAILING
};

Curl::Curl()
    :m_hdrState(HEADER_START)
    ,m_receivedData()
    ,m_etag()
    ,m_contentType()
    ,m_lastModified()
    ,m_effectiveUrl()
    ,m_userAgent()
    ,m_protocol()
    ,m_statusCode(0)
    ,m_permanentRedirectUrl()
    ,m_cacheControlMaxAge(0)
    ,m_age(0)
{
    // Ensure curl_global_init is called only once
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });

    // Initialize the CURL handle
    m_curl = curl_easy_init();
}

Curl::~Curl() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

long Curl::__post(const std::string& url, std::chrono::milliseconds timeout, const std::optional<std::string> &content, const std::map<std::string, std::string> &request_headers)
{
    if(!m_curl) return -2;

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 500l);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeout.count());
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_receivedData);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    if (m_userAgent.empty()) {
        curl_easy_setopt(m_curl, CURLOPT_USERAGENT, MBSF_TYPE "/" MBSF_VERSION);
     } else {
         curl_easy_setopt(m_curl, CURLOPT_USERAGENT, m_userAgent.c_str());
     }
     struct curl_slist *hdrs_list = nullptr;
     for (const auto &[field, val] : request_headers) {
         hdrs_list = curl_slist_append(hdrs_list, std::format("{}: {}", field, val).c_str());
     }
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, hdrs_list);
    curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
    if(content.has_value()) {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, content.value().c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, content.value().size());
    }

    CURLcode res = curl_easy_perform(m_curl);

    if (hdrs_list) {
        curl_slist_free_all(hdrs_list);
        hdrs_list = nullptr;
    }
    if (res == CURLE_OK) {
        // Get the Content-Type header
            char *ct = NULL;
            res = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &ct);
            if (!res && ct) {
                m_contentType = std::string(ct);
            }
	    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_statusCode);

            return m_receivedData.size(); // Return the number of bytes received
    
    } else if (res == CURLE_OPERATION_TIMEDOUT) {
            return -1; // Indicate timeout
    } else {
            //std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return -2; // Indicate other error
    }
    return -2;

}

long Curl::post(const std::string& url, std::chrono::milliseconds timeout, const std::optional<std::string> &content, const std::optional<std::string> &content_type)
{
    std::map<std::string, std::string> req_hdrs;

    if (content_type) {
        req_hdrs.insert(std::make_pair(std::string("Content-Type"), std::string(content_type.value())));
    }
    return __post(url, timeout, content, req_hdrs);
}

long Curl::__get(const std::string& url, std::chrono::milliseconds timeout, const std::map<std::string, std::string> &request_headers)
{
    m_etag.clear();                              // Clear the ETag before making a new request
    m_receivedData.clear();                      // Clear the received data before making a new request
    m_lastModified = decltype(m_lastModified){}; // Reset the last modified date-time to the epoch before making a new request
    m_age = 0;                                   // Zero the current object age before making a new request

    if (m_curl) {
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
        curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 500l);
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeout.count());
        curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_receivedData);
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
        if (m_userAgent.empty()) {
            curl_easy_setopt(m_curl, CURLOPT_USERAGENT, MBSF_TYPE "/" MBSF_VERSION);
        } else {
            curl_easy_setopt(m_curl, CURLOPT_USERAGENT, m_userAgent.c_str());
        }
        struct curl_slist *hdrs_list = nullptr;
        for (const auto &[field, val] : request_headers) {
            hdrs_list = curl_slist_append(hdrs_list, std::format("{}: {}", field, val).c_str());
        }
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, hdrs_list);

        // reset values collected during redirects
        m_hdrState = HEADER_START;
        m_statusCode = 0;
        m_protocol.clear();
        m_permanentRedirectUrl.clear();

        CURLcode res = curl_easy_perform(m_curl);

        if (hdrs_list) {
            curl_slist_free_all(hdrs_list);
            hdrs_list = nullptr;
        }
        if (res == CURLE_OK) {
            struct curl_header *type;
            CURLHcode h;

            h = curl_easy_header(m_curl, "ETag", 0, CURLH_HEADER, -1, &type);
            if (h == CURLHE_OK && type) {
                m_etag = std::string(type->value);
                ogs_debug("ETag: %s", m_etag.c_str());
            } else {
                ogs_debug("ETag header not found.");
            }

            h = curl_easy_header(m_curl, "Last-Modified", 0, CURLH_HEADER, -1, &type);
            if (h == CURLHE_OK && type) {
                m_lastModified = http_datetime_str_to_time_point(std::string(type->value));
                ogs_debug("%s", std::format("Last-Modified: {}", m_lastModified).c_str());
            } else {
                ogs_debug("Last-Modified header not found.");
            }

            h = curl_easy_header(m_curl, "Age", 0, CURLH_HEADER, -1, &type);
            if (h == CURLHE_OK && type) {
                try {
                    m_age = std::stoul(type->value);
                    ogs_debug("%s", std::format("Age: %lu", m_age).c_str());
                } catch (const std::exception&) {
                }
            }

            // Get the Content-Type header
            char *ct = NULL;
            res = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &ct);
            if (!res && ct) {
                m_contentType = std::string(ct);
            }

            char *redir_url = NULL;
            res = curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &redir_url);
            if (!res && redir_url) {
                m_effectiveUrl = redir_url;
            }

            return m_receivedData.size(); // Return the number of bytes received
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            return -1; // Indicate timeout
        } else {
            //std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return -2; // Indicate other error
        }
    }
    return -2; // Indicate error if m_curl is not initialized
}



long Curl::get(const std::string& url, std::chrono::milliseconds timeout, const std::optional<date_time_type> &last_modified, const std::optional<std::string> &etag)
{
    std::map<std::string, std::string> req_hdrs;

    if (etag) {
        req_hdrs.insert(std::make_pair(std::string("If-None-Match"), std::string(etag.value())));
    }
    if (last_modified) {
        req_hdrs.insert(std::make_pair(std::string("If-Modified-Since"), time_point_to_http_datetime_str(last_modified.value())));
    }
    return __get(url, timeout, req_hdrs);
}

std::vector<unsigned char>& Curl::getData()
{
    return m_receivedData;
}

const std::vector<unsigned char>& Curl::getData() const
{
    return m_receivedData;
}

const std::string& Curl::getEtag() const
{
    return m_etag;
}

const std::string& Curl::getContentType() const
{
    return m_contentType;
}

const Curl::date_time_type &Curl::getLastModified() const
{
    return m_lastModified;
}

const std::string &Curl::getEffectiveUrl() const
{
    return m_effectiveUrl;
}

const std::string &Curl::getPermanentRedirectUrl() const
{
    return m_permanentRedirectUrl;
}

unsigned long Curl::getCacheControlMaxAge() const
{
    return m_cacheControlMaxAge;
}

unsigned long Curl::getAge() const
{
    return m_age;
}

int Curl::getResponseCode() const
{
    return m_statusCode;
}

bool Curl::extractProtocolAndStatusCode(std::string_view &header_line)
{
    header_line.remove_prefix(5); // Skip "HTTP/"
    // Find first space character marking the end of the protocol version
    auto end_of_protocol = header_line.find_first_of(' ');
    if (end_of_protocol == std::string_view::npos) return false;
    m_protocol = header_line.substr(0, end_of_protocol);

    // Skip to start of status code
    header_line.remove_prefix(end_of_protocol);
    auto start_of_statuscode = header_line.find_first_not_of(' ');
    if (start_of_statuscode == std::string_view::npos) return false;
    header_line.remove_prefix(start_of_statuscode);

    // Find the end of the status code, clip and convert the status code into an integer
    auto end_of_statuscode = header_line.find_first_of(' ');
    if (end_of_statuscode == std::string_view::npos) {
        m_statusCode = std::stoi(std::string(header_line));
    } else {
        m_statusCode = std::stoi(std::string(header_line.substr(0,end_of_statuscode)));
    }
    return true;
}

void Curl::processHeaderLine(std::string_view &header_line)
{
    // Strip trailing CR LF
    if (header_line.ends_with("\r\n")) header_line.remove_suffix(2);

    switch (m_hdrState) {
    case HEADER_START: // haven't had any headers yet, so waiting for status line
        //ogs_debug("Status line: %s", std::string(header_line).c_str());
        if (!header_line.starts_with("HTTP/")) break;
        if (!extractProtocolAndStatusCode(header_line)) break;
        m_hdrState = HEADER_HEADERS;
        break;
    case HEADER_HEADERS: // had status line, now processing headers until we see an empty line
        //ogs_debug("Header: %s", std::string(header_line).c_str());
        if (header_line.empty()) {
            m_hdrState = HEADER_BODY;
        } else if ((m_statusCode == 301 || m_statusCode == 308) &&
                   (header_line.starts_with("Location:") || header_line.starts_with("location:"))) {
            header_line.remove_prefix(9);
            while (header_line.front() == ' ') header_line.remove_prefix(1);
            m_permanentRedirectUrl = header_line;
            ogs_debug("Got new redirect URL: %s", m_permanentRedirectUrl.c_str());
        } else if (header_line.starts_with("Cache-Control:") || header_line.starts_with("cache-control:")|| header_line.starts_with("Cache-control:")) {
            // Remove "Cache-Control:" prefix (14 characters)
            header_line.remove_prefix(14);
            // Skip any leading whitespace after the colon
            while (!header_line.empty() && header_line.front() == ' ')
                header_line.remove_prefix(1);

            // Look for the "max-age=" directive within the header line.
            auto pos = header_line.find("max-age=");
            if (pos == std::string_view::npos) {
                pos = header_line.find("Max-age=");
            }
            if (pos != std::string_view::npos) {
                pos += 8; // Move past "max-age="
                // Find where the numeric value ends (comma, space, or end-of-line)
                auto endPos = header_line.find_first_of(", \r\n", pos);
                if (endPos == std::string_view::npos)
                    endPos = header_line.size();

                // Convert the extracted substring to an integer.
                std::string maxAgeStr(header_line.substr(pos, endPos - pos));
                try {
                    m_cacheControlMaxAge = std::stoul(maxAgeStr);
                } catch (const std::exception&) {
                    m_cacheControlMaxAge = 0; // Fallback in case of conversion failure
                }
                ogs_debug("Extracted Cache-Control max-age: %lu", m_cacheControlMaxAge);
            }
        }
        break;
    case HEADER_BODY: // We're in the response body, so this is either a trailing header or a new status line after redirect
        //ogs_debug("Trailer/New status: %s", std::string(header_line).c_str());
        if (header_line.starts_with("HTTP/") && extractProtocolAndStatusCode(header_line)) {
            m_hdrState = HEADER_HEADERS;
        } else {
            m_hdrState = HEADER_TRAILING;
        }
        break;
    case HEADER_TRAILING: // We're in trailing headers, but if we see a status line we're starting a new redirected fetch
        if (header_line.starts_with("HTTP/") && extractProtocolAndStatusCode(header_line)) {
            m_hdrState = HEADER_HEADERS;
        }
        break;
    default:
        break;
    }
}

size_t Curl::headerCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    size_t totalSize = size * nitems;
    Curl* self = reinterpret_cast<Curl*>(userdata);

    std::string_view header_line(buffer, totalSize);
    self->processHeaderLine(header_line);

    return totalSize;
}

size_t Curl::writeCallback(void* contents, size_t memberSize, size_t numberOfMembers, void* userData)
{
    size_t totalSize = memberSize * numberOfMembers;
    std::vector<unsigned char>* receivedData = static_cast<std::vector<unsigned char>*>(userData);
    unsigned char* data = static_cast<unsigned char*>(contents);
    receivedData->insert(receivedData->end(), data, data + totalSize);
    return totalSize;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

