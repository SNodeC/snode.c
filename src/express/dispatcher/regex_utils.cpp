/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "express/dispatcher/regex_utils.h"

#include "express/Controller.h"
#include "express/MountPoint.h"
#include "express/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    inline bool isParamNameChar(const char c) {
        const unsigned char uc = static_cast<unsigned char>(c);
        return (std::isalnum(uc) != 0) || (c == '_');
    }

    // Express v4 string route patterns need regex matching when they contain any of these tokens.
    inline bool routeNeedsRegex(std::string_view path) {
        for (const char c : path) {
            switch (c) {
                case ':':
                case '*':
                case '?':
                case '+':
                case '(':
                case ')':
                case '\\':
                    return true;
                default:
                    break;
            }
        }
        return false;
    }

    inline bool isRegexMetaToEscape(const char c) {
        switch (c) {
            case '.':
            case '^':
            case '$':
            case '|':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '*':
            case '+':
            case '?':
            case '\\':
                return true;
            default:
                return false;
        }
    }

    inline void appendEscapedLiteral(std::string& out, const char c) {
        if (isRegexMetaToEscape(c)) {
            out.push_back('\\');
        }
        out.push_back(c);
    }

    // Extract inner text of a balanced "(...)" starting at openPos, handling nesting, escapes, and char-classes.
    inline bool extractBalancedParenInner(std::string_view s, const std::size_t openPos, std::string& innerOut, std::size_t& closePosOut) {
        if (openPos >= s.size() || s[openPos] != '(') {
            return false;
        }

        bool escaped = false;
        bool inCharClass = false;
        int depth = 0;

        for (std::size_t i = openPos; i < s.size(); ++i) {
            const char c = s[i];

            if (escaped) {
                escaped = false;
                continue;
            }

            if (c == '\\') {
                escaped = true;
                continue;
            }

            if (inCharClass) {
                if (c == ']') {
                    inCharClass = false;
                }
                continue;
            }

            if (c == '[') {
                inCharClass = true;
                continue;
            }

            if (c == '(') {
                ++depth;
                continue;
            }

            if (c == ')') {
                --depth;
                if (depth == 0) {
                    closePosOut = i;
                    innerOut.assign(s.substr(openPos + 1, closePosOut - openPos - 1));
                    return true;
                }
                continue;
            }
        }

        return false;
    }

    // Convert capturing groups inside a user-provided regex fragment into non-capturing groups.
    // Keeps submatch numbering stable so only :params and splats map to req.params.
    inline std::string makeInnerGroupsNonCapturing(std::string_view pattern) {
        std::string out;
        out.reserve(pattern.size());

        bool escaped = false;
        bool inCharClass = false;

        for (std::size_t i = 0; i < pattern.size(); ++i) {
            const char c = pattern[i];

            if (escaped) {
                out.push_back(c);
                escaped = false;
                continue;
            }

            if (c == '\\') {
                out.push_back(c);
                escaped = true;
                continue;
            }

            if (inCharClass) {
                out.push_back(c);
                if (c == ']') {
                    inCharClass = false;
                }
                continue;
            }

            if (c == '[') {
                out.push_back(c);
                inCharClass = true;
                continue;
            }

            if (c == '(') {
                // Preserve special constructs like (?:...), (?=...), (?!...), (?<=...), etc.
                if ((i + 1) < pattern.size() && pattern[i + 1] == '?') {
                    out.push_back('(');
                } else {
                    out.append("(?:");
                }
                continue;
            }

            out.push_back(c);
        }

        return out;
    }

    static bool matchAndFillParamsAndConsume(const std::regex& rx,
                                             const std::vector<std::string>& names,
                                             std::string_view reqPath,
                                             express::Request& req,
                                             std::size_t& consumedLength) {
        std::cmatch m;
        if (!std::regex_search(reqPath.begin(), reqPath.end(), m, rx)) {
            consumedLength = 0;
            return false;
        }
        consumedLength = static_cast<std::size_t>(m.length(0));
        const size_t g = (!m.empty()) ? (m.size() - 1) : 0;
        const size_t n = std::min(names.size(), g);
        for (size_t i = 0; i < n; ++i) {
            if (!names[i].empty() && m[i + 1].matched) {
                req.params[names[i]] = m[i + 1].str();
            }
        }
        return true;
    }

    std::unordered_map<std::string, std::string> parseQuery(std::string_view qs) {
        std::unordered_map<std::string, std::string> m;
        std::size_t i = 0;
        while (i < qs.size()) {
            const std::size_t amp = qs.find('&', i);
            const std::string_view pair = (amp == std::string_view::npos) ? qs.substr(i) : qs.substr(i, amp - i);
            const std::size_t eq = pair.find('=');
            std::string key;
            std::string val;
            if (eq == std::string_view::npos) {
                key.assign(pair);
            } else {
                key.assign(pair.substr(0, eq));
                val.assign(pair.substr(eq + 1));
            }
            if (!key.empty()) {
                m.emplace(std::move(key), std::move(val));
            }
            if (amp == std::string_view::npos) {
                break;
            }
            i = amp + 1;
        }
        return m;
    }

    static std::pair<std::regex, std::vector<std::string>>
    compileParamRegex(std::string_view mountPath, bool isPrefix, bool strictRouting, bool caseInsensitive) {
        // Express v4 string route patterns support:
        //  - named params:           /users/:id
        //  - custom param patterns:  /users/:id(\d+)
        //  - param modifiers:        ?, +, *   e.g. /:id?  /:path+  /:path*
        //  - wildcards:              *         e.g. /ab*cd  /hello/world(/*)?
        //  - grouping:               ()        (converted to non-capturing by default)
        //  - operators:              ? +       (regex-like, per Express examples)

        std::string rx;
        rx.reserve(mountPath.size() * 2U + 32U);
        rx.push_back('^');

        std::vector<std::string> names;
        names.reserve(4);

        std::size_t splatIndex = 0;

        for (std::size_t i = 0; i < mountPath.size();) {
            const char c = mountPath[i];

            // Backslash escapes the next char literally.
            if (c == '\\') {
                if (i + 1 < mountPath.size()) {
                    appendEscapedLiteral(rx, mountPath[i + 1]);
                    i += 2;
                } else {
                    appendEscapedLiteral(rx, c);
                    ++i;
                }
                continue;
            }

            // Named parameter: :name, optionally :name(<regex>) and modifier ? + *
            if (c == ':') {
                std::size_t j = i + 1;
                while (j < mountPath.size() && isParamNameChar(mountPath[j])) {
                    ++j;
                }

                if (j == i + 1) {
                    // Lone ':' -> treat as literal.
                    appendEscapedLiteral(rx, c);
                    ++i;
                    continue;
                }

                std::string name(mountPath.substr(i + 1, j - (i + 1)));
                std::string paramPattern = "[^/]+";

                // Custom regex in parentheses: :name(<pattern>)
                if (j < mountPath.size() && mountPath[j] == '(') {
                    std::string inner;
                    std::size_t closePos = 0;
                    if (extractBalancedParenInner(mountPath, j, inner, closePos)) {
                        paramPattern = makeInnerGroupsNonCapturing(inner);
                        j = closePos + 1;
                    }
                }

                char modifier = '\0';
                if (j < mountPath.size() && (mountPath[j] == '?' || mountPath[j] == '+' || mountPath[j] == '*')) {
                    modifier = mountPath[j];
                    ++j;
                }

                // If the previous output ended with '/', include the delimiter in optional/repeat
                // to emulate Express segment-parameter behaviour.
                const bool hasLeadingSlash = (!rx.empty() && rx.back() == '/');

                if (hasLeadingSlash && (modifier == '?' || modifier == '+' || modifier == '*')) {
                    rx.pop_back();

                    if (modifier == '?') {
                        rx.append("(?:/(");
                        rx.append(paramPattern);
                        rx.append("))?");
                    } else if (modifier == '+') {
                        rx.append("/(");
                        rx.append(paramPattern);
                        rx.append("(?:/");
                        rx.append(paramPattern);
                        rx.append(")*)");
                    } else { // '*'
                        rx.append("(?:/(");
                        rx.append(paramPattern);
                        rx.append("(?:/");
                        rx.append(paramPattern);
                        rx.append(")*))?");
                    }
                } else {
                    if (modifier == '?') {
                        rx.append("(");
                        rx.append(paramPattern);
                        rx.append(")?");
                    } else if (modifier == '+') {
                        rx.append("(");
                        rx.append(paramPattern);
                        rx.append("(?:/");
                        rx.append(paramPattern);
                        rx.append(")*)");
                    } else if (modifier == '*') {
                        rx.append("(");
                        rx.append(paramPattern);
                        rx.append("(?:/");
                        rx.append(paramPattern);
                        rx.append(")*)?");
                    } else {
                        rx.append("(");
                        rx.append(paramPattern);
                        rx.append(")");
                    }
                }

                names.emplace_back(std::move(name));
                i = j;
                continue;
            }

            // Express wildcard token '*': matches any sequence including '/'.
            if (c == '*') {
                rx.append("(.*)");
                names.emplace_back(std::to_string(splatIndex++)); // Express-style numeric params
                ++i;
                continue;
            }

            // Grouping: convert to non-capturing by default to keep capture numbering stable.
            if (c == '(') {
                if (i + 1 < mountPath.size() && mountPath[i + 1] == '?') {
                    // Keep special groups as-is.
                    rx.push_back('(');
                } else {
                    rx.append("(?:");
                }
                ++i;
                continue;
            }

            if (c == ')') {
                rx.push_back(')');
                ++i;
                continue;
            }

            // Operators allowed by Express v4 string routes.
            if (c == '?' || c == '+') {
                rx.push_back(c);
                ++i;
                continue;
            }

            // Ordinary literal.
            appendEscapedLiteral(rx, c);
            ++i;
        }

        if (isPrefix) {
            rx.append("(?:/|$)"); // boundary prefix
        } else {
            if (!strictRouting) {
                rx.append("/?"); // allow single trailing slash
            }
            rx.push_back('$');
        }

        std::regex::flag_type flags = std::regex::ECMAScript;
        if (caseInsensitive) {
            flags |= std::regex::icase;
        }

        try {
            std::regex re(rx, flags);
            // Cache sentinel: allow caching even for regex routes without capture groups.
            if (names.empty()) {
                names.emplace_back("__compiled");
            }
            return {std::move(re), std::move(names)};
        } catch (const std::regex_error&) {
            std::regex re("a^", flags); // match-nothing
            if (names.empty()) {
                names.emplace_back("__compiled");
            }

            return {std::move(re), std::move(names)};
        }
    }

    static bool querySupersetMatches(const std::unordered_map<std::string, std::string>& rq,
                                     const std::unordered_map<std::string, std::string>& need) {
        for (const auto& kv : need) {
            auto it = rq.find(kv.first);
            if (it == rq.end() || it->second != kv.second) {
                return false;
            }
        }
        return true;
    }

    static bool boundaryPrefix(std::string_view path, std::string_view base, bool caseInsensitive) {
        // Normalize: an empty base is equivalent to "/"
        if (base.empty()) {
            base = "/";
        }

        // Special case: base "/" matches any absolute path
        if (base.size() == 1 && base[0] == '/') {
            return !path.empty() && path.front() == '/';
        }

        // Base longer than path cannot match
        if (base.size() > path.size()) {
            return false;
        }

        auto eq = [&](char a, char b) {
            return !caseInsensitive ? (a == b)
                                    : (std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)));
        };

        // Check prefix characters
        for (size_t i = 0; i < base.size(); ++i) {
            if (!eq(path[i], base[i])) {
                return false;
            }
        }

        // Boundary: either exact match, or next char is a '/'
        return (path.size() == base.size()) || (path[base.size()] == '/');
    }

    static bool equalPath(std::string_view a, std::string_view b, bool caseInsensitive) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (!caseInsensitive ? (a[i] != b[i]) : !ieq(a[i], b[i])) {
                return false;
            }
        }
        return true;
    }

    static std::string_view trimOneTrailingSlash(std::string_view s) {
        if (s.size() > 1 && s.back() == '/') {
            return std::string_view(s.data(), s.size() - 1);
        }
        return s;
    }

    static void splitMountPathAndQuery(std::string_view url, std::string_view& path, std::string_view& query) {
        // Mount points can optionally specify required query pairs (e.g. "/foo?x=1&y=2").
        // Express-style route wildcards also use '?' as an operator (e.g. "/ab?cd").
        // Heuristic: only treat '?' as query delimiter if the tail looks like a query string.
        const std::size_t qpos = url.find('?');
        if (qpos == std::string_view::npos) {
            path = url;
            query = {};
            return;
        }

        const std::string_view tail = url.substr(qpos + 1);
        if (tail.find('=') == std::string_view::npos && tail.find('&') == std::string_view::npos) {
            // Treat as route pattern (not a query-string spec)
            path = url;
            query = {};
            return;
        }

        path = url.substr(0, qpos);
        query = tail;
    }

    void splitPathAndQuery(std::string_view url, std::string_view& path, std::string_view& query) {
        const std::size_t qpos = url.find('?');
        if (qpos == std::string_view::npos) {
            path = url;
            query = {};
        } else {
            path = url.substr(0, qpos);
            query = url.substr(qpos + 1);
        }
    }

    bool methodMatches(std::string_view requestMethod, const std::string& mountMethod) {
        return (mountMethod == "use") || (mountMethod == "all") || (requestMethod == mountMethod);
    }

    static MountMatchResult matchMountPointImpl(express::Controller& controller,
                                                const std::string& absoluteMountPath,
                                                const express::MountPoint& mountPoint,
                                                std::regex* cachedRegex,
                                                std::vector<std::string>* cachedNames) {
        MountMatchResult result;
        result.isPrefix = (mountPoint.method == "use");

        // Split mount & request into path + query
        std::string_view mountPath;
        std::string_view mountQueryString;
        splitMountPathAndQuery(absoluteMountPath, mountPath, mountQueryString);
        const auto requiredQueryPairs = parseQuery(mountQueryString);

        std::string_view requestPath;
        std::string_view requestQueryString;
        splitPathAndQuery(controller.getRequest()->originalUrl, requestPath, requestQueryString);
        result.requestQueryPairs = parseQuery(requestQueryString);

        // Normalize single trailing slash if not strict
        if (!controller.getStrictRouting()) {
            mountPath = trimOneTrailingSlash(mountPath);
            requestPath = trimOneTrailingSlash(requestPath);
        }
        if (mountPath.empty()) {
            mountPath = "/";
        }

        result.requestPath = requestPath;

        bool pathMatches = false;
        std::size_t matchLen = 0;

        if (routeNeedsRegex(mountPath)) {
            // Param mount: optionally compile once, match once, fill params, and record matched prefix length
            if (cachedRegex != nullptr && cachedNames != nullptr) {
                if (cachedNames->empty()) {
                    auto compiled = compileParamRegex(mountPath,
                                                      /*isPrefix*/ result.isPrefix,
                                                      controller.getStrictRouting(),
                                                      controller.getCaseInsensitiveRouting());
                    *cachedRegex = std::move(compiled.first);
                    *cachedNames = std::move(compiled.second);
                }
                pathMatches = matchAndFillParamsAndConsume(*cachedRegex, *cachedNames, requestPath, *controller.getRequest(), matchLen);
            } else {
                auto [rx, names] = compileParamRegex(mountPath,
                                                     /*isPrefix*/ result.isPrefix,
                                                     controller.getStrictRouting(),
                                                     controller.getCaseInsensitiveRouting());
                pathMatches = matchAndFillParamsAndConsume(rx, names, requestPath, *controller.getRequest(), matchLen);
            }

            if (pathMatches && result.isPrefix) {
                result.consumedLength = matchLen;
            }
        } else {
            if (result.isPrefix) {
                // Literal boundary prefix
                pathMatches = boundaryPrefix(requestPath, mountPath, controller.getCaseInsensitiveRouting());
                if (pathMatches) {
                    result.consumedLength = mountPath.size();
                }
            } else {
                // End-anchored equality
                pathMatches = equalPath(requestPath, mountPath, controller.getCaseInsensitiveRouting());
            }
        }

        const bool queryMatches = querySupersetMatches(result.requestQueryPairs, requiredQueryPairs);
        result.requestMatched = (pathMatches && queryMatches);

        return result;
    }

    MountMatchResult
    matchMountPoint(express::Controller& controller, const std::string& absoluteMountPath, const express::MountPoint& mountPoint) {
        return matchMountPointImpl(controller, absoluteMountPath, mountPoint, nullptr, nullptr);
    }

    MountMatchResult matchMountPoint(express::Controller& controller,
                                     const std::string& absoluteMountPath,
                                     const express::MountPoint& mountPoint,
                                     std::regex& cachedRegex,
                                     std::vector<std::string>& cachedNames) {
        return matchMountPointImpl(controller, absoluteMountPath, mountPoint, &cachedRegex, &cachedNames);
    }

    ScopedPathStrip::ScopedPathStrip(express::Request& req, std::string_view requestPath, bool enabled, std::size_t consumedLength)
        : req_(&req)
        , enabled_(enabled) {
        if (!enabled_) {
            return;
        }

        backup_ = req.path;

        std::string_view remainderPath{};
        if (requestPath.size() > consumedLength) {
            remainderPath = requestPath.substr(consumedLength);
            if (!remainderPath.empty() && remainderPath.front() == '/') {
                remainderPath.remove_prefix(1);
            }
        }

        req.path = remainderPath.empty() ? "/" : ("/" + std::string(remainderPath));
    }

    ScopedPathStrip::~ScopedPathStrip() {
        if (enabled_ && req_ != nullptr) {
            req_->path = backup_;
        }
    }

} // namespace express::dispatcher
