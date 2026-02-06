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

#ifndef EXPRESS_DISPATCHER_REGEX_UTILS_H
#define EXPRESS_DISPATCHER_REGEX_UTILS_H

namespace express {
    class Request;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    std::vector<std::string> explode(const std::string& s, char delim);

    const std::regex& pathRegex();

    std::smatch matchResult(const std::string& cpath);

    bool hasResult(const std::string& cpath);

    void setParams(const std::string& cpath, express::Request& req);

    // ---------- URL split & query parsing ----------
    void splitPathAndQuery(std::string_view url, std::string_view& path, std::string_view& query);

    std::unordered_map<std::string, std::string> parseQuery(std::string_view qs);

    // ---------- path normalization & comparisons ----------
    std::string_view trimOneTrailingSlash(std::string_view s);

    inline bool ieq(char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    }

    bool equalPath(std::string_view a, std::string_view b, bool caseInsensitive);

    // prefix with **segment boundary** (so "/api" won't match "/apix")
    bool boundaryPrefix(std::string_view path, std::string_view base, bool caseInsensitive);

    bool querySupersetMatches(const std::unordered_map<std::string, std::string>& rq,
                              const std::unordered_map<std::string, std::string>& need);

    // ---------- param path → regex compiler ----------
    // Converts "/api/:id(\\d+)/files/:rest(.*)" into:
    //   ^/api/(\d+)/files/(.*)(?:/|$)        (prefix mode, router/middleware)
    //   ^/api/(\d+)/files/(.*)/?$            (end-anchored (strict=false), application)
    // and returns capture group names in order: ["id","rest"]
    std::pair<std::regex, std::vector<std::string>> compileParamRegex(std::string_view mountPath,
                                                                      bool isPrefix, // router/middleware=true, application=false
                                                                      bool strictRouting,
                                                                      bool caseInsensitive);

    template <typename RequestLike>
    inline bool
    matchAndFillParams(const std::regex& rx, const std::vector<std::string>& names, std::string_view reqPath, RequestLike& req) {
        std::cmatch m;
        if (!std::regex_search(reqPath.begin(), reqPath.end(), m, rx)) {
            return false;
        }
        const size_t g = (!m.empty()) ? (m.size() - 1) : 0;
        const size_t n = std::min(names.size(), g);
        for (size_t i = 0; i < n; ++i) {
            req.params[names[i]] = m[i + 1].str();
        }
        return true;
    }


    template <typename RequestLike>
    inline bool matchAndFillParamsAndConsume(const std::regex& rx,
                                             const std::vector<std::string>& names,
                                             std::string_view reqPath,
                                             RequestLike& req,
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
            req.params[names[i]] = m[i + 1].str();
        }
        return true;
    }

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_REGEX_UTILS_H
