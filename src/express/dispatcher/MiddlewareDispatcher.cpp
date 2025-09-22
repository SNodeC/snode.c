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

#include "express/dispatcher/MiddlewareDispatcher.h"

#include "core/socket/stream/SocketConnection.h"
#include "express/Next.h"
#include "express/Request.h"
#include "express/Response.h"
#include "express/Route.h"
#include "express/dispatcher/regex_utils.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <list>
#include <string_view>
#include <unordered_map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    MiddlewareDispatcher::MiddlewareDispatcher(
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, express::Next&)>& lambda)
        : lambda(lambda) {
    }

    bool MiddlewareDispatcher::dispatch(express::Controller& controller, const std::string& parentMountPath, const MountPoint& mountPoint) {
        bool requestMatched = false;

        if ((controller.getFlags() & Controller::NEXT) == 0) {
            using namespace express::detail;

            const std::string absoluteMountPath = parentMountPath + mountPoint.relativeMountPath;

            const std::string requestMethod = controller.getRequest()->method;
            const std::string requestUrl = controller.getRequest()->url;
            const std::string requestPath = controller.getRequest()->path;

            // Split mount & request into path + query
            std::string_view mPath;
            std::string_view mQs;
            splitPathAndQuery(absoluteMountPath, mPath, mQs);
            auto needQ = parseQuery(mQs);

            std::string_view reqAbs;
            std::string_view reqQs;
            splitPathAndQuery(controller.getRequest()->originalUrl, reqAbs, reqQs);
            auto rq = parseQuery(reqQs);

            // Normalize single trailing slash if not strict
            if (!controller.getStrictRouting()) {
                mPath = trimOneTrailingSlash(mPath);
                reqAbs = trimOneTrailingSlash(reqAbs);
            }
            if (mPath.empty()) {
                mPath = "/";
            }

            // Middleware is **prefix** with boundary (Express: app.use)
            bool pathOk = false;
            if (absoluteMountPath.find(':') != std::string::npos) {
                auto [rx, names] = compile_param_regex(mPath,
                                                       /*isPrefix*/ true,
                                                       controller.getStrictRouting(),
                                                       controller.getCaseInsensitiveRouting());
                pathOk = match_and_fill_params(rx, names, reqAbs, *controller.getRequest());
            } else {
                pathOk = boundaryPrefix(reqAbs, mPath, controller.getCaseInsensitiveRouting());
            }
            const bool queryOk = querySupersetMatches(rq, needQ);
            requestMatched = (pathOk && queryOk);
            if (!requestMatched) {
                return requestMatched;
            }

            // Optional params on the mount (e.g. "/api/:tenant")
            if (absoluteMountPath.find(':') != std::string::npos) {
                auto [rx, names] =
                    compile_param_regex(mPath, /*isPrefix*/ true, controller.getStrictRouting(), controller.getCaseInsensitiveRouting());
                if (!match_and_fill_params(rx, names, reqAbs, *controller.getRequest())) {
                    requestMatched = false;
                    return requestMatched;
                }
            }

            // Compute remainder and temporarily **rewrite req.path**
            std::string_view rem{};
            if (reqAbs.size() > mPath.size()) {
                rem = reqAbs.substr(mPath.size());
                if (!rem.empty() && rem.front() == '/') {
                    rem.remove_prefix(1);
                }
            }
            auto& req = *controller.getRequest();
            const std::string prevPath = req.path;
            req.path = rem.empty() ? "/" : ("/" + std::string(rem));

            req.queries.insert(rq.begin(), rq.end());

            LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                       << " HTTP Express: middleware -> " << (requestMatched ? "MATCH" : "NO MATCH");
            LOG(TRACE) << "      RequestMethod: " << controller.getRequest()->method;
            LOG(TRACE) << "         RequestUrl: " << controller.getRequest()->url;
            LOG(TRACE) << "        RequestPath: " << controller.getRequest()->path;
            LOG(TRACE) << "  AbsoluteMountPath: " << absoluteMountPath;
            LOG(TRACE) << "      StrictRouting: " << controller.getStrictRouting();
            LOG(TRACE) << "      StrictRouting: " << controller.getStrictRouting();

            if (requestMatched) {
                if (hasResult(absoluteMountPath)) {
                    setParams(absoluteMountPath, *controller.getRequest());
                }

                Next next(controller);
                lambda(controller.getRequest(), controller.getResponse(), next);

                // If next() was called synchronously continue current route-tree traversal
                if ((next.controller.getFlags() & express::Controller::NEXT) != 0) {
                    LOG(TRACE) << "Express: M - Next called - set to NO MATCH";
                    requestMatched = false;
                    controller = next.controller;
                }
            }

            // Restore
            req.path = prevPath;
        }

        return requestMatched;
    }

    std::list<std::string>
    MiddlewareDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const {
        std::list<std::string> routes{"M " + parentMountPath + mountPoint.relativeMountPath + (!strictRouting ? "*" : "")};
        routes.push_back("  " + mountPoint.method + " " + mountPoint.relativeMountPath);

        if (nextRoute) {
            routes.splice(routes.end(), nextRoute->getRoute(parentMountPath, strictRouting));
        }

        return routes;
    }

} // namespace express::dispatcher
