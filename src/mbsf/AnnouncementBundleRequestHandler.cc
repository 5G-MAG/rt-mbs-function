/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: request handler of the announcement bundle paths
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "UserServiceDiscoveryHttp.hh"

#include "AnnouncementBundleRequestHandler.hh"

HTTPXPP_NAMESPACE_USING(HTTPRequest);
HTTPXPP_NAMESPACE_USING(HTTPResponse);
HTTPXPP_NAMESPACE_USING(HTTPServer);

MBSF_NAMESPACE_START

HTTPResponse AnnouncementBundleRequestHandler::doRequest(const HTTPRequest &request, const HTTPServer &server)
{
    HTTPResponse response = [&]() -> HTTPResponse {
        if (const auto id = bundleSessionId(request.url())) {
            /* The name has no slash and no leading dot and a symbolic link is not followed, so the directory is one of
               the docroot's own. Anything else, a missing directory included, is the docroot handler's to answer. */
            const std::filesystem::path dir = std::filesystem::path(m_docroot->docroot()) / *id;
            std::error_code ec;
            if (std::filesystem::symlink_status(dir, ec).type() == std::filesystem::file_type::directory) {
                return m_index->makeResponseForDir(dir.string() + "/", request.url(), server,
                                                   request.getHeaderFirst("If-None-Match"),
                                                   request.getHeaderFirst("If-Modified-Since"));
            }
        }
        return m_docroot->doRequest(request, server);
    }();

    /* TS 26.517 V19.2.0 clause 8.2.3.4: a response that carries a resource message body includes "a predicted
       time-to-live period for the resource, conveyed in a Cache-Control: max-age response header". */
    if ((response.statusCode() == 200 || response.statusCode() == 304) && !response.getHeader("Cache-Control")) {
        response.addHeader("Cache-Control",
                           discovery::cacheControlMaxAge(App::self().context()->cacheControl.UserServiceDescriptionMaxAge));
    }

    return response;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
