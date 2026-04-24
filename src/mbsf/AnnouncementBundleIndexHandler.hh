#ifndef _MBSF_ANNOUNCEMENT_BUNDLE_INDEX_HANDLER_HH_
#define _MBSF_ANNOUNCEMENT_BUNDLE_INDEX_HANDLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: AnnouncementBundleIndexHandler class
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

#include <string>
#include <vector>

#include "common.hh"
#include <DocrootHTTPRequestHandler.hh>

HTTPXPP_NAMESPACE_START
class HTTPResponse;
class HTTPServer;
HTTPXPP_NAMESPACE_STOP

MBSF_NAMESPACE_START

class AnnouncementBundleIndexHandler : public HTTPXPP_NAMESPACE_NAME(DocrootHTTPRequestHandler::IndexHandler) {
public:
    AnnouncementBundleIndexHandler() :HTTPXPP_NAMESPACE_NAME(DocrootHTTPRequestHandler::IndexHandler)() {};
    AnnouncementBundleIndexHandler(const AnnouncementBundleIndexHandler &other) = delete;
    AnnouncementBundleIndexHandler(AnnouncementBundleIndexHandler &&other) = delete;

    virtual ~AnnouncementBundleIndexHandler() {};

    AnnouncementBundleIndexHandler &operator=(const AnnouncementBundleIndexHandler &other) = delete;
    AnnouncementBundleIndexHandler &operator=(AnnouncementBundleIndexHandler &&other) = delete;

    virtual HTTPXPP_NAMESPACE_NAME(HTTPResponse) makeResponseForDir(const std::string &dir_path, const std::string &url_path, const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server);
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_ANNOUNCEMENT_BUNDLE_INDEX_HANDLER_HH_ */
