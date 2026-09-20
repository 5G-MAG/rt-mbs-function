#ifndef __CONDITIONAL_REQUEST_HH__
#define __CONDITIONAL_REQUEST_HH__
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: conditional request evaluation
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

#include <string>
#include <optional>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <string_view>

#include "common.hh"

MBSF_NAMESPACE_START

/** Strip the optional weakness indicator and the quotes from an entity-tag.
 *
 * RFC 9110 section 8.8.3: “entity-tag = [ weak ] opaque-tag” and “opaque-tag = DQUOTE *etagc
 * DQUOTE”. Comparison is over the opaque-tag, so the delimiters are removed before comparing.
 */
inline std::string_view entityTagOpaque(std::string_view tag)
{
    if (tag.size() >= 2 && tag.compare(0, 2, "W/") == 0) tag.remove_prefix(2);
    if (tag.size() >= 2 && tag.front() == '"' && tag.back() == '"') {
        tag.remove_prefix(1);
        tag.remove_suffix(1);
    }
    return tag;
}

/** Whether an If-Match or If-None-Match field value lists @p etag.
 *
 * "*" matches any existing representation. Otherwise the field is a comma separated list of
 * entity-tags, compared on their opaque-tags.
 *
 * The distinction RFC 9110 draws between strong comparison for If-Match (section 13.1.1) and weak
 * comparison for If-None-Match (section 13.1.2) does not arise here: this MBSF never emits a weak
 * tag, so no stored tag can be weak and the two comparisons coincide.
 */
/** Parse an HTTP-date into a time point.
 *
 * RFC 9110 section 5.6.7 requires a recipient to accept all three formats; the preferred IMF-fixdate
 * is tried first, then the obsolete RFC 850 and asctime forms. Returns nullopt if none parse, which
 * RFC 9110 section 13.1.3 says to treat as no condition at all rather than as a failed one.
 */
inline std::optional<std::time_t> parseHttpDate(const std::string &value)
{
    static const char * const formats[] = {
        "%a, %d %b %Y %H:%M:%S",    /* IMF-fixdate */
        "%A, %d-%b-%y %H:%M:%S",    /* RFC 850     */
        "%a %b %d %H:%M:%S %Y"      /* asctime     */
    };
    for (const char *fmt : formats) {
        std::tm tm{};
        std::istringstream in(value);
        in >> std::get_time(&tm, fmt);
        if (in.fail()) continue;
        /* HTTP-dates are always GMT, so the fields are interpreted as UTC rather than local time. */
        return timegm(&tm);
    }
    return std::nullopt;
}

/** True when the resource was last modified no later than the date the client offered.
 *
 * RFC 9110 section 13.1.3: a recipient answers 304 when the selected representation has not been
 * modified since the given date. An unparseable date on either side answers false, so the request
 * proceeds normally and the client gets the representation.
 */
inline bool notModifiedSince(const std::string &last_modified, const std::string &if_modified_since)
{
    auto lm = parseHttpDate(last_modified);
    auto ims = parseHttpDate(if_modified_since);
    if (!lm || !ims) return false;
    return *lm <= *ims;
}

inline bool entityTagListMatches(std::string_view field_value, std::string_view etag)
{
    const auto wanted = entityTagOpaque(etag);
    size_t pos = 0;
    while (pos <= field_value.size()) {
        auto comma = field_value.find(',', pos);
        auto item = field_value.substr(pos, comma == std::string_view::npos
                                                ? std::string_view::npos : comma - pos);
        while (!item.empty() && (item.front() == ' ' || item.front() == '\t')) item.remove_prefix(1);
        while (!item.empty() && (item.back() == ' ' || item.back() == '\t')) item.remove_suffix(1);
        if (item == "*") return true;
        if (!item.empty() && !wanted.empty() && entityTagOpaque(item) == wanted) return true;
        if (comma == std::string_view::npos) break;
        pos = comma + 1;
    }
    return false;
}

/** What a conditional request asks the handler to do instead of the method. */
enum class Precondition {
    Proceed,          //!< no applicable condition, or it evaluated true
    NotModified,      //!< If-None-Match matched on a GET: answer 304
    PreconditionFailed //!< If-Match did not match: answer 412 and do not perform the method
};

/** Evaluate the conditional request headers against the current entity-tag.
 *
 * \param if_match         The If-Match field value, empty when the header is absent.
 * \param if_none_match    The If-None-Match field value, empty when the header is absent.
 * \param etag             The current entity-tag of the target resource.
 * \param is_safe_method   True for GET and HEAD, which is what makes a match a 304 rather than 412.
 *
 * RFC 9110 section 13.1.1: “An origin server that evaluates an If-Match condition MUST NOT perform
 * the requested method if the condition evaluates to false.”
 *
 * Section 13.1.2 requires the mirror of that for If-None-Match, answering 304 for a safe method.
 *
 * If-Match is evaluated first: section 13.2.2 gives the order in which preconditions are evaluated,
 * and If-Match precedes If-None-Match.
 */
inline Precondition evaluatePreconditions(std::string_view if_match, std::string_view if_none_match,
                                          std::string_view etag, bool is_safe_method)
{
    if (!if_match.empty() && !entityTagListMatches(if_match, etag)) {
        return Precondition::PreconditionFailed;
    }
    if (!if_none_match.empty() && entityTagListMatches(if_none_match, etag)) {
        /* A safe method gets the cached representation confirmed; anything else is a lost update
           being prevented, which is a failed precondition. */
        return is_safe_method ? Precondition::NotModified : Precondition::PreconditionFailed;
    }
    return Precondition::Proceed;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* __CONDITIONAL_REQUEST_HH__ */
