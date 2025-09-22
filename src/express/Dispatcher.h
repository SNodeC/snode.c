/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef EXPRESS_DISPATCHER_H
#define EXPRESS_DISPATCHER_H

namespace express {
    class Route;
    class Controller;
    struct MountPoint;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <list>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::detail {

    // ---------- URL split & query parsing ----------
    inline void splitPathAndQuery(std::string_view url, std::string_view& path, std::string_view& query) {
        const std::size_t qpos = url.find('?');
        if (qpos == std::string_view::npos) {
            path = url;
            query = {};
        } else {
            path = url.substr(0, qpos);
            query = url.substr(qpos + 1);
        }
    }

    inline std::unordered_map<std::string, std::string> parseQuery(std::string_view qs) {
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

    // ---------- path normalization & comparisons ----------
    inline std::string_view trimOneTrailingSlash(std::string_view s) {
        if (s.size() > 1 && s.back() == '/') {
            return std::string_view(s.data(), s.size() - 1);
        }
        return s;
    }

    inline bool ieq(char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    }

    inline bool equalPath(std::string_view a, std::string_view b, bool caseInsensitive) {
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

    // prefix with **segment boundary** (so "/api" won't match "/apix")
    inline bool boundaryPrefix(std::string_view path, std::string_view base, bool caseInsensitive) {
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
            return !caseInsensitive ? (a == b) : (std::tolower((unsigned char) a) == std::tolower((unsigned char) b));
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

    inline bool querySupersetMatches(const std::unordered_map<std::string, std::string>& rq,
                                     const std::unordered_map<std::string, std::string>& need) {
        for (const auto& kv : need) {
            auto it = rq.find(kv.first);
            if (it == rq.end() || it->second != kv.second) {
                return false;
            }
        }
        return true;
    }

    // ---------- param path → regex compiler ----------
    // Converts "/api/:id(\\d+)/files/:rest(.*)" into:
    //   ^/api/(\d+)/files/(.*)(?:/|$)        (prefix mode, router/middleware)
    //   ^/api/(\d+)/files/(.*)/?$            (end-anchored (strict=false), application)
    // and returns capture group names in order: ["id","rest"]
    inline std::pair<std::regex, std::vector<std::string>> compile_param_regex(std::string_view mountPath,
                                                                               bool isPrefix, // router/middleware=true, application=false
                                                                               bool strictRouting,
                                                                               bool caseInsensitive) {
        std::string pat;
        pat.reserve(mountPath.size() * 2);
        std::vector<std::string> names;
        pat += '^';

        const char* s = mountPath.data();
        const char* e = s + mountPath.size();
        while (s < e) {
            if (*s == ':') {
                ++s;
                const char* nstart = s;
                while (s < e && (std::isalnum((unsigned char) *s) > 0 || *s == '_' || *s == '-')) {
                    ++s;
                }
                std::string name(nstart, s);
                std::string custom;
                if (s < e && *s == '(') {
                    int depth = 0;
                    const char* rstart = s + 1;
                    ++s;
                    while (s < e) {
                        if (*s == '(') {
                            ++depth;
                        } else if (*s == ')' && depth-- == 0) {
                            break;
                        }
                        ++s;
                    }
                    custom.assign(rstart, s);
                    if (s < e && *s == ')') {
                        ++s;
                    }
                }
                names.push_back(std::move(name));
                if (!custom.empty()) {
                    pat += '(';
                    pat += custom;
                    pat += ')';
                } else {
                    pat += "([^/]+)";
                } // default: single segment
            } else {
                static const std::string meta = R"(\.^$|()[]{}*+?!)";
                if (meta.find(*s) != std::string::npos) {
                    pat += '\\';
                }
                pat += *s;
                ++s;
            }
        }

        if (isPrefix) {
            pat += "(?:/|$)"; // boundary
        } else {
            if (!strictRouting) {
                pat += "/?"; // allow single trailing slash
            }
            pat += '$';
        }

        auto flags = std::regex::ECMAScript | (!caseInsensitive ? std::regex::flag_type{} : std::regex::icase);

        return {std::regex(pat, flags), std::move(names)};
    }

    template <class RequestLike>
    inline bool
    match_and_fill_params(const std::regex& rx, const std::vector<std::string>& names, std::string_view reqPath, RequestLike& req) {
        std::cmatch m;
        if (!std::regex_search(reqPath.begin(), reqPath.end(), m, rx)) {
            return false;
        }
        const size_t g = (m.size() > 0) ? (m.size() - 1) : 0;
        const size_t n = std::min(names.size(), g);
        for (size_t i = 0; i < n; ++i) {
            req.params[names[i]] = m[i + 1].str();
        }
        return true;
    }

} // namespace express::detail

namespace express {

    class Dispatcher {
    public:
        Dispatcher(const Dispatcher&) = delete;

        Dispatcher& operator=(const Dispatcher&) = delete;

        Dispatcher() = default;

        virtual ~Dispatcher();

        virtual bool dispatch(Controller& controller, const std::string& parentMountPath, const MountPoint& mountPoint) = 0;
        bool dispatchNext(Controller& controller, const std::string& parentMountPath);

        virtual std::list<std::string>
        getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const = 0;

    protected:
        std::shared_ptr<Route> nextRoute = nullptr;

    private:
        friend class Route;
    };

} // namespace express

#endif // EXPRESS_DISPATCHER_H
