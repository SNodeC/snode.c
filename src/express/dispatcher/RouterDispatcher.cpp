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
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string_view>
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
        using namespace express::detail;

        bool dispatched = false;

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

        // Router is **prefix** with boundary (like app.use)
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

        bool requestMatched = (pathOk && queryOk);
        if (!requestMatched) {
            return requestMatched;
        }
        // Optional params on the mount ("/api/:tenant")
        if (absoluteMountPath.find(':') != std::string::npos) {
            auto [rx, names] =
                compile_param_regex(mPath, /*isPrefix*/ true, controller.getStrictRouting(), controller.getCaseInsensitiveRouting());
            if (!match_and_fill_params(rx, names, reqAbs, *controller.getRequest())) {
                requestMatched = false;
                return requestMatched;
            }
        }

        // Compute remainder and temporarily **rewrite req.path** for subtree
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
                   << " HTTP Express: router -> " << (requestMatched ? "MATCH" : "NO MATCH");
        LOG(TRACE) << "      RequestMethod: " << controller.getRequest()->method;
        LOG(TRACE) << "         RequestUrl: " << controller.getRequest()->url;
        LOG(TRACE) << "        RequestPath: " << controller.getRequest()->path;
        LOG(TRACE) << "  AbsoluteMountPath: " << absoluteMountPath;
        LOG(TRACE) << "      StrictRouting: " << controller.getStrictRouting();
        LOG(TRACE) << "      StrictRouting: " << controller.getStrictRouting();

        if (requestMatched) {
            for (Route& route : routes) {
                const bool oldStrictRouting = controller.setStrictRouting(strictRouting);

                dispatched = route.dispatch(controller, absoluteMountPath);

                controller.setStrictRouting(oldStrictRouting);

                if (dispatched) {
                    LOG(TRACE) << "Express: R - Dispatched: " << dispatched;

                    break;
                }
                if (controller.nextRouterCalled()) {
                    LOG(TRACE) << "Express: R - NextRouter called - breaking dispatching";

                    break;
                }
            }
        }

        req.path = prevPath;

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
