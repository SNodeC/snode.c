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

#include "express/dispatcher/ApplicationDispatcher.h"

#include "core/socket/stream/SocketConnection.h"
#include "express/Controller.h"
#include "express/Request.h"
#include "express/Response.h"
#include "express/Route.h"
#include "express/dispatcher/regex_utils.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <cstddef>
#include <list>
#include <string_view>
#include <tuple>
#include <unordered_map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    ApplicationDispatcher::ApplicationDispatcher(
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda)
        : lambda(lambda) {
    }

    bool ApplicationDispatcher::dispatch(express::Controller& controller,
                                         const std::string& parentMountPath,
                                         const express::MountPoint& mountPoint) {
        bool requestMatched = false;

        if (controller.getRequest()->method == mountPoint.method || mountPoint.method == "use" || mountPoint.method == "all") {
            const std::string absoluteMountPath = parentMountPath + mountPoint.relativeMountPath;
            const bool isUse = (mountPoint.method == "use");
            const bool isPrefix = isUse; // Express: only app.use() is prefix-based

            if ((controller.getFlags() & Controller::NEXT) == 0) {
                // Split mount & request into path + query
                std::string_view mountPath;
                std::string_view mountQueryString;
                splitPathAndQuery(absoluteMountPath, mountPath, mountQueryString);
                const auto requiredQueryPairs = parseQuery(mountQueryString);

                std::string_view requestPath;
                std::string_view requestQueryString;
                splitPathAndQuery(controller.getRequest()->originalUrl, requestPath, requestQueryString);
                const auto requestQueryPairs = parseQuery(requestQueryString);

                // Normalize single trailing slash if not strict
                if (!controller.getStrictRouting()) {
                    mountPath = trimOneTrailingSlash(mountPath);
                    requestPath = trimOneTrailingSlash(requestPath);
                }
                if (mountPath.empty()) {
                    mountPath = "/";
                }

                // Matching is independent of callback arity:
                //   - use() => prefix with boundary
                //   - verbs/all() => end-anchored equality
                bool pathMatches = false;
                std::size_t consumedLength = 0;
                if (mountPath.find(':') != std::string::npos) {
                    // Param mount: compile once, match once, fill params, and record matched prefix length (use only)
                    if (regex.mark_count() == 0) {
                        LOG(TRACE) << "ApplicationDispatcher: precompiled regex";
                        std::tie(regex, names) = compileParamRegex(mountPath,
                                                                   /*isPrefix*/ isPrefix,
                                                                   controller.getStrictRouting(),
                                                                   controller.getCaseInsensitiveRouting());
                    } else {
                        LOG(TRACE) << "ApplicationDispatcher: using precompiled regex";
                    }
                    std::size_t matchLen = 0;
                    pathMatches = matchAndFillParamsAndConsume(regex, names, requestPath, *controller.getRequest(), matchLen);
                    if (pathMatches && isPrefix) {
                        consumedLength = matchLen;
                    }
                } else {
                    if (isPrefix) {
                        // Literal boundary prefix
                        pathMatches = boundaryPrefix(requestPath, mountPath, controller.getCaseInsensitiveRouting());
                        if (pathMatches) {
                            consumedLength = mountPath.size();
                        }
                    } else {
                        // End-anchored equality
                        pathMatches = equalPath(requestPath, mountPath, controller.getCaseInsensitiveRouting());
                    }
                }

                const bool queryMatches = querySupersetMatches(requestQueryPairs, requiredQueryPairs);
                requestMatched = (pathMatches && queryMatches);

                LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                           << " HTTP Express: application -> " << (requestMatched ? "MATCH" : "NO MATCH");
                LOG(TRACE) << "           RequestMethod: " << controller.getRequest()->method;
                LOG(TRACE) << "              RequestUrl: " << controller.getRequest()->url;
                LOG(TRACE) << "             RequestPath: " << controller.getRequest()->path;
                LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
                LOG(TRACE) << " Mountpoint RelativePath: " << mountPoint.relativeMountPath;
                LOG(TRACE) << " Mountpoint AbsolutePath: " << absoluteMountPath;
                LOG(TRACE) << "           StrictRouting: " << controller.getStrictRouting();
                LOG(TRACE) << "  CaseInsensitiveRouting: " << controller.getCaseInsensitiveRouting();

                if (requestMatched) {
                    auto& req = *controller.getRequest();
                    req.queries.insert(requestQueryPairs.begin(), requestQueryPairs.end());

                    // Express-style mount path stripping is only applied for use()
                    const std::string previousPathBackup = req.path;
                    if (isPrefix) {
                        std::string_view remainderPath{};
                        if (requestPath.size() > consumedLength) {
                            remainderPath = requestPath.substr(consumedLength);
                            if (!remainderPath.empty() && remainderPath.front() == '/') {
                                remainderPath.remove_prefix(1);
                            }
                        }
                        req.path = remainderPath.empty() ? "/" : ("/" + std::string(remainderPath));
                    }

                    // NOTE: do not run legacy setParams() here; it can overwrite regex-extracted params
                    lambda(controller.getRequest(), controller.getResponse());

                    if (isPrefix) {
                        req.path = previousPathBackup;
                    }
                }
            } else {
                LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                           << " HTTP Express: application -> next(...) called";
                LOG(TRACE) << "           RequestMethod: " << controller.getRequest()->method;
                LOG(TRACE) << "              RequestUrl: " << controller.getRequest()->url;
                LOG(TRACE) << "             RequestPath: " << controller.getRequest()->path;
                LOG(TRACE) << "       AbsoluteMountPath: " << absoluteMountPath;
            }
        }

        return requestMatched;
    }

    std::list<std::string>
    ApplicationDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const {
        std::list<std::string> routes{"A " + parentMountPath + mountPoint.relativeMountPath + (!strictRouting ? "*" : "")};
        routes.push_back("  " + mountPoint.method + " " + mountPoint.relativeMountPath);

        if (nextRoute) {
            routes.splice(routes.end(), nextRoute->getRoute(parentMountPath, strictRouting));
        }

        return routes;
    }

} // namespace express::dispatcher
