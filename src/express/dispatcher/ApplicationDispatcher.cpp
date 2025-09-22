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

#include <list>
#include <string_view>
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

            // End-anchored equality (verbs) with one trailing slash relaxed when !strict
            std::string_view mNorm = mPath;
            std::string_view pNorm = reqAbs;
            if (!controller.getStrictRouting()) {
                mNorm = trimOneTrailingSlash(mNorm);
                pNorm = trimOneTrailingSlash(pNorm);
            }
            if (mNorm.empty()) {
                mNorm = "/";
            }

            bool pathOk = false;
            if (mPath.find(':') != std::string::npos) {
                auto [rx, names] = compile_param_regex(mPath,
                                                       /*isPrefix*/ false,
                                                       controller.getStrictRouting(),
                                                       controller.getCaseInsensitiveRouting());
                pathOk = match_and_fill_params(rx, names, reqAbs, *controller.getRequest());
            } else {
                pathOk = equalPath(pNorm, mNorm, controller.getCaseInsensitiveRouting());
            }

            const bool queryOk = querySupersetMatches(rq, needQ);
            requestMatched = (pathOk && queryOk);
            if (!requestMatched) {
                return requestMatched;
            }

            // Optional params on the application route (e.g. "/users/:id(\\d+)")
            if (absoluteMountPath.find(':') != std::string::npos) {
                auto [rx, names] =
                    compile_param_regex(mPath, /*isPrefix*/ false, controller.getStrictRouting(), controller.getCaseInsensitiveRouting());
                if (!match_and_fill_params(rx, names, reqAbs, *controller.getRequest())) {
                    requestMatched = false;
                    return requestMatched;
                }
            }

            controller.getRequest()->queries.insert(rq.begin(), rq.end());

            LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                       << " HTTP Express: application -> " << (requestMatched ? "MATCH" : "NO MATCH");
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

                lambda(controller.getRequest(), controller.getResponse());
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
