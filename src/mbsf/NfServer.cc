/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Network Function Server
 ******************************************************************************
 * Copyright: (C) 2024 British Broadcasting Corporation
 * Author: David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License (v1.0)
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

#include "ogs-sbi.h"

#include <format>
#include <map>
#include <sstream>
#include <string>

#include "common.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "openapi/model/CJson.hh"
#include "mbsf-version.h"

#include "NfServer.hh"

using fiveg_mag_reftools::CJson;

MBSF_NAMESPACE_START

static Open5GSSBIResponse new_response(const NfServer::AppMetadata &app,
                                       const std::optional<NfServer::InterfaceMetadata> &interface = std::nullopt,
                                       const std::optional<std::string> &location = std::nullopt,
                                       const std::optional<std::string> &content_type = std::nullopt,
                                       const std::optional<NfServer::time_type> &last_modified = std::nullopt,
                                       const std::optional<std::string> &etag = std::nullopt, int cache_control_max_age = 0,
                                       const std::optional<std::string> &allow_methods = std::nullopt);
static bool send_problem(Open5GSSBIStream &stream, OpenAPI_problem_details_t *problem,
                         const std::optional<NfServer::InterfaceMetadata> &interface, const NfServer::AppMetadata &app);
static Open5GSSBIResponse build_response(Open5GSSBIMessage &message, int status,
                                         const std::optional<NfServer::InterfaceMetadata> &interface,
                                         const NfServer::AppMetadata &app);
static bool build_content(ogs_sbi_http_message_t *http, Open5GSSBIMessage &message);
static char *build_json(Open5GSSBIMessage &message);

// NfServer::InterfaceMetadata class

NfServer::InterfaceMetadata::InterfaceMetadata(const std::string &api_title, const std::string &api_version)
    :m_apiTitle(api_title)
    ,m_apiVersion(api_version)
{
}

NfServer::InterfaceMetadata::InterfaceMetadata(const char *api_title, const char *api_version)
    :m_apiTitle(api_title)
    ,m_apiVersion(api_version)
{
}

NfServer::InterfaceMetadata::InterfaceMetadata(const NfServer::InterfaceMetadata &other)
    :m_apiTitle(other.apiTitle())
    ,m_apiVersion(other.apiVersion())
{
}

NfServer::InterfaceMetadata::~InterfaceMetadata()
{
}

NfServer::InterfaceMetadata &NfServer::InterfaceMetadata::operator=(const NfServer::InterfaceMetadata &other)
{
    m_apiTitle = other.apiTitle();
    m_apiVersion = other.apiVersion();
    return *this;
}

// NfServer::AppMetadata class

NfServer::AppMetadata::AppMetadata(const std::string &app_name, const std::string &app_version, const std::string &server_name)
    :m_appName(app_name)
    ,m_appVersion(app_version)
    ,m_serverName(server_name)
{
}

NfServer::AppMetadata::AppMetadata(const char *app_name, const char *app_version, const char *server_name)
    :m_appName(app_name)
    ,m_appVersion(app_version)
    ,m_serverName(server_name)
{
}

NfServer::AppMetadata::~AppMetadata()
{
}

// NfServer class

bool NfServer::sendError(Open5GSSBIStream &stream, int status, size_t number_of_components,
                         const Open5GSSBIMessage &message, const AppMetadata &app,
                         const std::optional<InterfaceMetadata> &interface,
                         const std::optional<std::string> &title, const std::optional<std::string> &detail,
                         const std::optional<CJson> &problem_detail_json,
                         const std::optional<std::map<std::string,std::string> > &invalid_params,
                         const std::optional<std::string> &problem_type)
{
    return __sendError(stream, status, std::nullopt, number_of_components, message, app, interface, title, detail, problem_detail_json, invalid_params, problem_type);
}

bool NfServer::sendError(Open5GSSBIStream &stream, const fiveg_mag_reftools::ProblemCause &cause, size_t number_of_components,
                         const Open5GSSBIMessage &message, const AppMetadata &app,
                         const std::optional<InterfaceMetadata> &interface,
                         const std::optional<std::string> &title, const std::optional<std::string> &detail,
                         const std::optional<CJson> &problem_detail_json,
                         const std::optional<std::map<std::string,std::string> > &invalid_params,
                         const std::optional<std::string> &problem_type)
{
    return __sendError(stream, cause.statusCode(), cause, number_of_components, message, app, interface, title?title:cause.reason(), detail, problem_detail_json, invalid_params, problem_type);
}

bool NfServer::__sendError(Open5GSSBIStream &stream, int status, const std::optional<fiveg_mag_reftools::ProblemCause> &cause,
                         size_t number_of_components,
                         const Open5GSSBIMessage &message, const AppMetadata &app,
                         const std::optional<InterfaceMetadata> &interface,
                         const std::optional<std::string> &title, const std::optional<std::string> &detail,
                         const std::optional<CJson> &problem_detail_json,
                         const std::optional<std::map<std::string,std::string> > &invalid_params,
                         const std::optional<std::string> &problem_type)
{
    OpenAPI_problem_details_t *problem = OpenAPI_problem_details_create(nullptr, nullptr, false, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    OpenAPI_problem_details_t *problem_details = NULL;

    ogs_assert(stream);

    if (problem_detail_json) {
        CJson copy(problem_detail_json.value());
        cJSON *copy_impl(copy.exportCJSON());
        problem_details = OpenAPI_problem_details_parseFromJSON(copy_impl);
        problem->invalid_params = problem_details->invalid_params;
        problem_details->invalid_params = NULL;
        OpenAPI_problem_details_free(problem_details);
        cJSON_Delete(copy_impl);
    }

    if (cause) {
        problem->cause = ogs_strdup(cause->cause().c_str());
    }

    if (message) {
        if (problem_type) {
            problem->type = ogs_strdup(problem_type.value().c_str());
            ogs_expect(problem->type);
        }

        if (invalid_params) {
            if (problem->invalid_params) {
                OpenAPI_lnode_t *node;
                OpenAPI_list_for_each(problem->invalid_params, node) {
                    OpenAPI_invalid_param_free(reinterpret_cast<OpenAPI_invalid_param_t*>(node->data));
                }
                OpenAPI_list_clear(problem->invalid_params);
            }
            if (invalid_params->size() > 0) {
                if (!problem->invalid_params) {
                    problem->invalid_params = OpenAPI_list_create();
                }
                for (const auto& [key, value] : invalid_params.value()) {
                    OpenAPI_invalid_param_t *invalid_param = OpenAPI_invalid_param_create(ogs_strdup(key.c_str()), ogs_strdup(value.c_str()));
                    OpenAPI_list_add(problem->invalid_params, invalid_param);
                }
            } else {
                if (problem->invalid_params) {
                    OpenAPI_list_free(problem->invalid_params);
                    problem->invalid_params = nullptr;
                }
            }
        }

        std::ostringstream ostr;
        ostr << "/" << message.serviceName() << "/" << message.apiVersion();
        for (size_t i = 0; i <= number_of_components; i++) {
            const char *path_comp = message.resourceComponent(i);
            if (!path_comp) break;
            ostr << "/" << path_comp;
        }
        problem->instance = ogs_strdup(ostr.str().c_str());
    }

    if (status) {
        problem->is_status = true;
        problem->status = status;
    }

    if (title) problem->title = ogs_strdup(title->c_str());
    if (detail) problem->detail = ogs_strdup(detail->c_str());

    send_problem(stream, problem, interface, app);
    return true;
}

std::shared_ptr<Open5GSSBIResponse> NfServer::newResponse(const std::optional<std::string> &location,
                                        const std::optional<std::string> &content_type,
                                        const std::optional<NfServer::time_type> &last_modified,
                                        const std::optional<std::string> &etag, int cache_control_max_age,
                                        const std::optional<std::string> &allow_methods,
                                        const std::optional<InterfaceMetadata> &interface, const AppMetadata &app)
{
    return std::shared_ptr<Open5GSSBIResponse>(new Open5GSSBIResponse(new_response(app, interface, location, content_type,
                                                                                   last_modified, etag, cache_control_max_age,
                                                                                   allow_methods)));
}

std::shared_ptr<Open5GSSBIResponse> NfServer::populateResponse(std::shared_ptr<Open5GSSBIResponse> &response, const std::string &content, int status)
{
    response->contentLength(content.size());
    response->content(ogs_strdup(content.c_str()));
    response->status(status);
    return response;
}

std::map<std::string, std::string> NfServer::makeInvalidParams(const std::string &param, const std::string &reason)
{
    std::map<std::string, std::string> retval;

    retval.insert(std::make_pair(std::string(param), std::string(reason)));

    return retval;
}

// NfServer class private methods

static Open5GSSBIResponse new_response(const NfServer::AppMetadata &app,
                                       const std::optional<NfServer::InterfaceMetadata> &interface,
                                       const std::optional<std::string> &location,
                                       const std::optional<std::string> &content_type,
                                       const std::optional<NfServer::time_type> &last_modified,
                                       const std::optional<std::string> &etag, int cache_control_max_age,
                                       const std::optional<std::string> &allow_methods)
{
    ogs_sbi_response_t *response = NULL;

    response = ogs_sbi_response_new();
    ogs_expect(response);

    if (content_type) {
        ogs_sbi_header_set(response->http.headers, "Content-Type", content_type->c_str());
    }

    if (location) {
        ogs_sbi_header_set(response->http.headers, "Location", location->c_str());
    }

    if (last_modified) {
        std::string modified(std::format("{:%a, %d %b %Y %H:%M:%S %Z}", std::chrono::time_point_cast<std::chrono::seconds>(last_modified.value())));
        ogs_sbi_header_set(response->http.headers, "Last-Modified", modified.c_str());
    }

    if (etag) {
        ogs_sbi_header_set(response->http.headers, "ETag", etag->c_str());
    }

    if (cache_control_max_age > 0) {
        std::ostringstream response_cache_control;
        response_cache_control << "max-age=" << cache_control_max_age;
        ogs_sbi_header_set(response->http.headers, "Cache-Control", response_cache_control.str().c_str());
    }

    if (allow_methods) {
        ogs_sbi_header_set(response->http.headers, "Allow", allow_methods->c_str());
    }

    std::ostringstream server;

    server << app.serverName() << "/" << FIVEG_API_RELEASE << " ";
    if (interface) {
        server << "(info.title=" << interface->apiTitle() << "; info.version=" << interface->apiVersion() << ") ";
    }
    server << app.appName() << "/" << app.appVersion();

    ogs_sbi_header_set(response->http.headers, "Server", server.str().c_str());

    return Open5GSSBIResponse(response);
}

static bool send_problem(Open5GSSBIStream &stream, OpenAPI_problem_details_t *problem,
                         const std::optional<NfServer::InterfaceMetadata> &interface, const NfServer::AppMetadata &app)
{
    Open5GSSBIMessage message(new ogs_sbi_message_t({}), true);;
    char *content_type = ogs_strdup("application/problem+json");
    message.contentType(content_type);
    message.problemDetails(problem);

    Open5GSSBIResponse response(build_response(message, problem->status, interface, app));

    Open5GSSBIServer::sendResponse(stream, response);
    ogs_free(content_type);

    return true;
}


static Open5GSSBIResponse build_response(Open5GSSBIMessage &message, int status,
                                           const std::optional<NfServer::InterfaceMetadata> &interface, const NfServer::AppMetadata &app)
{
    Open5GSSBIResponse response(new_response(app, interface));

    response.status(status);

    if (status != OGS_SBI_HTTP_STATUS_NO_CONTENT) {
        ogs_expect(true == build_content(&response.ogsSBIResponse()->http, message));
    }

    if (message.location()) {
        response.headerSet("Location", message.location());
    }
    if (message.cacheControl()) {
        response.headerSet("Cache-Control", message.cacheControl());
    }

    return response;
}

static bool build_content(ogs_sbi_http_message_t *http, Open5GSSBIMessage &message)
{
    ogs_assert(http);

    char *content = build_json(message);
    if (content) {
        http->content = content;
        http->content_length = strlen(content);
        if (message.contentType()) {
            ogs_sbi_header_set(http->headers, OGS_SBI_CONTENT_TYPE, message.contentType());
        } else {
            ogs_sbi_header_set(http->headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
        }
    }

    return true;
}

static char *build_json(Open5GSSBIMessage &message)
{
    char *content = NULL;
    cJSON *item = NULL;

    if (message.problemDetails()) {
        item = OpenAPI_problem_details_convertToJSON(message.problemDetails());
        ogs_assert(item);
    }
    if (item) {
        content = cJSON_Print(item);
        ogs_assert(content);
        ogs_log_print(OGS_LOG_TRACE, "%s", content);
        cJSON_Delete(item);
    }

    return content;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
*/
