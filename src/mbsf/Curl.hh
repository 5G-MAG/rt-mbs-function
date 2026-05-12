#ifndef _MBSF_CURL_HH_
#define _MBSF_CURL_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: cURL client
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

#pragma once

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>
#include "common.hh"

MBSF_NAMESPACE_START

class Curl {
public:
    using date_time_type = std::chrono::system_clock::time_point;

    Curl();
    ~Curl();

    long get(const std::string& url, std::chrono::milliseconds timeout, const std::optional<date_time_type> &last_modified = std::nullopt, const std::optional<std::string> &etag = std::nullopt);
    long post(const std::string& url, std::chrono::milliseconds timeout, const std::optional<std::string> &content, const std::optional<std::string> &content_type);
    std::vector<unsigned char> &getData();
    const std::vector<unsigned char> &getData() const;
    const std::string &getEtag() const;
    const std::string &getContentType() const;
    const date_time_type &getLastModified() const;
    const std::string &getEffectiveUrl() const;
    const std::string &getPermanentRedirectUrl() const;
    unsigned long getCacheControlMaxAge() const;
    unsigned long getAge() const;
    int getResponseCode() const;

    Curl &setUserAgent(const std::string &user_agent);

private:
    long __get(const std::string& url, std::chrono::milliseconds timeout, const std::map<std::string,std::string> &request_headers);
    long __post(const std::string& url, std::chrono::milliseconds timeout, const std::optional<std::string> &content, const std::map<std::string, std::string> &request_headers);
    bool extractProtocolAndStatusCode(std::string_view &status_line);
    void processHeaderLine(std::string_view &header_line);
    static size_t headerCallback(char* buffer, size_t size, size_t numberOfItems, void* userData);
    static size_t writeCallback(void* contents, size_t memberSize, size_t numberOfMembers, void* userData);

    CURL* m_curl;
    int m_hdrState;
    std::vector<unsigned char> m_receivedData;
    std::string m_etag;
    std::string m_contentType;
    date_time_type m_lastModified;
    std::string m_effectiveUrl;
    std::string m_userAgent;
    std::string m_protocol;
    int m_statusCode;
    std::string m_permanentRedirectUrl;
    unsigned long m_cacheControlMaxAge;
    unsigned long m_age;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_CURL_HH_ */

