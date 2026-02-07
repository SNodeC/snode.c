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
    class Controller;
    struct MountPoint;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cctype>
#include <cstddef>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    inline bool ieq(char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    }

    // ---------- shared mount-point matching (used by Router/Application/Middleware dispatchers) ----------
    //
    // Supports Express v4 *string* route wildcards/operators: '*', '?', '+', and grouping '()',
    // plus ':param', ':param(<re>)' and ':param' modifiers ('?', '+', '*').

    bool methodMatches(std::string_view requestMethod, const std::string& mountMethod);

    struct MountMatchResult {
        bool requestMatched{false};
        bool isPrefix{false};
        std::size_t consumedLength{0};
        std::string_view requestPath;
        std::map<std::string, std::string> params;
        std::unordered_map<std::string, std::string> requestQueryPairs;
    };

    MountMatchResult
    matchMountPoint(express::Controller& controller, const std::string& absoluteMountPath, const express::MountPoint& mountPoint);

    MountMatchResult matchMountPoint(express::Controller& controller,
                                     const std::string& absoluteMountPath,
                                     const express::MountPoint& mountPoint,
                                     std::regex& cachedRegex,
                                     std::vector<std::string>& cachedNames);

    class ScopedPathStrip {
    public:
        ScopedPathStrip(express::Request& req, std::string_view requestUrl, bool enabled, std::size_t consumedLength);
        ~ScopedPathStrip();

        ScopedPathStrip(const ScopedPathStrip&) = delete;
        ScopedPathStrip& operator=(const ScopedPathStrip&) = delete;
        ScopedPathStrip(ScopedPathStrip&&) = delete;
        ScopedPathStrip& operator=(ScopedPathStrip&&) = delete;

    private:
        express::Request* req_{nullptr};
        std::string backupUrl_;
        std::string backupBaseUrl_;
        std::string backupPath_;
        std::string backupFile_;
        bool enabled_{false};
    };

    class ScopedParams {
    public:
        ScopedParams(express::Request& req, const std::map<std::string, std::string>& params, bool mergeWithParent);
        ~ScopedParams();

        ScopedParams(const ScopedParams&) = delete;
        ScopedParams& operator=(const ScopedParams&) = delete;
        ScopedParams(ScopedParams&&) = delete;
        ScopedParams& operator=(ScopedParams&&) = delete;

    private:
        express::Request* req_{nullptr};
        std::map<std::string, std::string> backup_;
    };

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_REGEX_UTILS_H
