#ifndef _MBSF_ANNOUNCEMENT_BUNDLE_REQUEST_HANDLER_HH_
#define _MBSF_ANNOUNCEMENT_BUNDLE_REQUEST_HANDLER_HH_
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

#include <memory>
#include <optional>
#include <string>

#include <DocrootHTTPRequestHandler.hh>
#include <HTTPRequest.hh>
#include <HTTPRequestHandler.hh>
#include <HTTPResponse.hh>
#include <HTTPServer.hh>

#include "common.hh"
#include "AnnouncementBundleIndexHandler.hh"

MBSF_NAMESPACE_START

/** Serves the MBS User Service Announcement bundles that the co-located server exposes from its docroot.
 *
 * The bundle of an Ingest Session is the directory URL "/{ingestSessionId}/" of the docroot. The docroot handler passes
 * the directory to an index handler that does not see the request, so this handler answers that one URL shape itself,
 * with the conditional request header fields, and gives every other request to the docroot handler. TS 26.517 V19.2.0
 * clause 8.2.3.4 asks of a response of the MBS AF at MBS-11 and of the MBSF at MBS-1 that carries a resource message body
 * a strong entity tag, a modification time and a Cache-Control max-age, and of the endpoints that they support
 * If-None-Match and If-Modified-Since: the index handler provides the validators and this handler the lifetime of the
 * files, to which the docroot handler already adds an entity tag and a modification time.
 */
class AnnouncementBundleRequestHandler : public HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler) {
public:
    AnnouncementBundleRequestHandler(const std::shared_ptr<HTTPXPP_NAMESPACE_NAME(DocrootHTTPRequestHandler)> &docroot,
                       const std::shared_ptr<AnnouncementBundleIndexHandler> &index)
        :HTTPXPP_NAMESPACE_NAME(HTTPRequestHandler)(), m_docroot(docroot), m_index(index) {}

    virtual void serverShutdown(const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server) { m_docroot->serverShutdown(server); }
    virtual HTTPXPP_NAMESPACE_NAME(HTTPResponse) doRequest(const HTTPXPP_NAMESPACE_NAME(HTTPRequest) &request,
                                                           const HTTPXPP_NAMESPACE_NAME(HTTPServer) &server);

    /** The Ingest Session identifier of a bundle URL, "/{ingestSessionId}/" and nothing else.
     *
     * A name that starts with a dot is refused, as the index handler refuses it, so that neither "." nor ".." nor a
     * hidden entry is taken for a session.
     */
    static std::optional<std::string> bundleSessionId(const std::string &url)
    {
        if (url.size() < 3 || url.front() != '/' || url.back() != '/') return std::nullopt;
        const std::string id = url.substr(1, url.size() - 2);
        if (id.find('/') != std::string::npos || id.front() == '.') return std::nullopt;
        return id;
    }

private:
    std::shared_ptr<HTTPXPP_NAMESPACE_NAME(DocrootHTTPRequestHandler)> m_docroot;
    std::shared_ptr<AnnouncementBundleIndexHandler> m_index;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_ANNOUNCEMENT_BUNDLE_REQUEST_HANDLER_HH_ */
