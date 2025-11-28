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

#include "express/dispatcher/RouterDispatcher.h"

#include "core/socket/stream/SocketConnection.h"
#include "express/Controller.h"
#include "express/Request.h"
#include "express/Response.h"
#include "express/Route.h"
#include "express/dispatcher/regex_utils.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <string_view>
#include <tuple>
#include <unordered_map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    std::list<Route>& RouterDispatcher::getRoutes() {
        return routes;
    }

    bool RouterDispatcher::setStrictRouting(bool strictRouting) {
        const bool oldStrictRouting = this->strictRouting;

        this->strictRouting = strictRouting;

        return oldStrictRouting;
    }

    bool RouterDispatcher::getStrictRouting() const {
        return strictRouting;
    }

    bool RouterDispatcher::setCaseInsensitiveRouting(bool caseInsensitiveRouting) {
        const bool oldCaseInsensitiveRouting = this->caseInsensitiveRouting;

        this->caseInsensitiveRouting = caseInsensitiveRouting;

        return oldCaseInsensitiveRouting;
    }

    bool RouterDispatcher::getCaseInsensitiveRouting() const {
        return caseInsensitiveRouting;
    }

    bool
    RouterDispatcher::dispatch(express::Controller& controller, const std::string& parentMountPath, const express::MountPoint& mountPoint) {
        bool dispatched = false;

        if (controller.getRequest()->method == mountPoint.method || mountPoint.method == "use" || mountPoint.method == "all") {
            const std::string absoluteMountPath = parentMountPath + mountPoint.relativeMountPath;

            // Split mount & request into path + query
            std::string_view mountPath;
            std::string_view mountQueryString;
            splitPathAndQuery(absoluteMountPath, mountPath, mountQueryString);
            const std::unordered_map<std::string, std::string> requiredQueryPairs = parseQuery(mountQueryString);

            std::string_view requestPath;
            std::string_view requestQueryString;
            splitPathAndQuery(controller.getRequest()->originalUrl, requestPath, requestQueryString);
            const std::unordered_map<std::string, std::string> requestQueryPairs = parseQuery(requestQueryString);

            // Normalize single trailing slash if not strict
            if (!controller.getStrictRouting()) {
                mountPath = trimOneTrailingSlash(mountPath);
                requestPath = trimOneTrailingSlash(requestPath);
            }
            if (mountPath.empty()) {
                mountPath = "/";
            }

            // Router is **prefix** with boundary (like app.use)
            bool pathMatches = false;
            if (absoluteMountPath.find(':') != std::string::npos) {
                if (regex.mark_count() == 0) {
                    LOG(TRACE) << "RouterDispatcher: precompiled regex";
                    std::tie(regex, names) = compileParamRegex(mountPath,
                                                               /*isPrefix*/ true,
                                                               controller.getStrictRouting(),
                                                               controller.getCaseInsensitiveRouting());
                } else {
                    LOG(TRACE) << "RouterDispatcher: using precompiled regex";
                }

                pathMatches = matchAndFillParams(regex, names, requestPath, *controller.getRequest());
            } else {
                pathMatches = boundaryPrefix(requestPath, mountPath, controller.getCaseInsensitiveRouting());
            }
            const bool queryMatches = querySupersetMatches(requestQueryPairs, requiredQueryPairs);

            const bool requestMatched = (pathMatches && queryMatches);

            LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                       << " HTTP Express: router -> " << (requestMatched ? "MATCH" : "NO MATCH");
            LOG(TRACE) << "           RequestMethod: " << controller.getRequest()->method;
            LOG(TRACE) << "              RequestUrl: " << controller.getRequest()->url;
            LOG(TRACE) << "             RequestPath: " << controller.getRequest()->path;
            LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
            LOG(TRACE) << " Mountpoint RelativePath: " << mountPoint.relativeMountPath;
            LOG(TRACE) << " Mountpoint AbsolutePath: " << absoluteMountPath;
            LOG(TRACE) << "           StrictRouting: " << controller.getStrictRouting();
            LOG(TRACE) << "  CaseInsensitiveRouting: " << controller.getCaseInsensitiveRouting();

            if (requestMatched) {
                // Compute remainder and temporarily **rewrite req.path** for subtree
                std::size_t consumedLength = 0;
                if (absoluteMountPath.find(':') != std::string::npos) {
                    // Run (or reuse) the param regex match and take its full-match length
                    std::cmatch regexMatches;
                    auto [regex, names] = compileParamRegex(
                        mountPath, /*isPrefix*/ true, controller.getStrictRouting(), controller.getCaseInsensitiveRouting());
                    if (!std::regex_search(requestPath.begin(), requestPath.end(), regexMatches, regex)) {
                        return false; // should not happen because pathMatches was true, but be safe
                    }
                    consumedLength = static_cast<std::size_t>(regexMatches.length(0)); // <-- ACTUAL matched prefix
                } else {
                    // Literal boundary prefix: consume exactly the base length
                    consumedLength = mountPath.size();
                }

                // Now compute remainder using 'consumedLength'
                std::string_view remainderPath{};
                if (requestPath.size() > consumedLength) {
                    remainderPath = requestPath.substr(consumedLength);
                    if (!remainderPath.empty() && remainderPath.front() == '/') {
                        remainderPath.remove_prefix(1);
                    }
                }

                auto& req = *controller.getRequest();
                const std::string previousPathBackup = req.path;
                req.path = remainderPath.empty() ? "/" : ("/" + std::string(remainderPath));

                req.queries.insert(requestQueryPairs.begin(), requestQueryPairs.end());

                for (Route& route : routes) {
                    const bool oldStrictRouting = controller.setStrictRouting(strictRouting);

                    dispatched = route.dispatch(controller, absoluteMountPath);

                    controller.setStrictRouting(oldStrictRouting);

                    if (dispatched) {
                        LOG(TRACE) << "Express: R - Dispatched";

                        break;
                    }
                    if (controller.nextRouterCalled()) {
                        LOG(TRACE) << "Express: R - NextRouter called - breaking dispatching";

                        break;
                    }
                }

                req.path = previousPathBackup;
            }
        }

        return dispatched;
    }

    std::list<std::string> RouterDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint) const {
        return getRoutes(parentMountPath, mountPoint, strictRouting);
    }

    std::list<std::string>
    RouterDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const {
        std::list<std::string> collectedRoutes;

        for (const Route& route : routes) {
            collectedRoutes.splice(
                collectedRoutes.end(),
                route.getRoute(parentMountPath + "$" + mountPoint.relativeMountPath + "$", this->strictRouting ? true : strictRouting));
        }

        return collectedRoutes;
    }

} // namespace express::dispatcher
