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

        const bool methodMatchesResult = methodMatches(controller.getRequest()->method, mountPoint.method);
        if (!methodMatchesResult) {
            return false;
        }

        const std::string absoluteMountPath = parentMountPath + mountPoint.relativeMountPath;

        if ((controller.getFlags() & Controller::NEXT) == 0) {
            const MountMatchResult match = matchMountPoint(controller, absoluteMountPath, mountPoint);

            LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                       << " HTTP Express: router -> " << (match.requestMatched ? "MATCH" : "NO MATCH");
            LOG(TRACE) << "           RequestMethod: " << controller.getRequest()->method;
            LOG(TRACE) << "              RequestUrl: " << controller.getRequest()->url;
            LOG(TRACE) << "             RequestPath: " << controller.getRequest()->path;
            LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
            LOG(TRACE) << " Mountpoint RelativePath: " << mountPoint.relativeMountPath;
            LOG(TRACE) << " Mountpoint AbsolutePath: " << absoluteMountPath;
            LOG(TRACE) << "           StrictRouting: " << controller.getStrictRouting();
            LOG(TRACE) << "  CaseInsensitiveRouting: " << controller.getCaseInsensitiveRouting();

            if (match.requestMatched) {
                auto& req = *controller.getRequest();
                req.queries.insert(match.requestQueryPairs.begin(), match.requestQueryPairs.end());

                // Express-style mount path stripping is only applied for use()
                const ScopedPathStrip pathStrip(req, req.originalUrl, match.isPrefix, match.consumedLength);
                const ScopedParams scopedParams(req, match.params, false);

                const bool oldStrictRouting = controller.setStrictRouting(strictRouting);
                const bool oldCaseInsensitiveRouting = controller.setCaseInsensitiveRouting(caseInsensitiveRouting);

                for (Route& route : routes) {
                    dispatched = route.dispatch(controller, absoluteMountPath);

                    if (dispatched) {
                        LOG(TRACE) << "Express: R - Dispatched";
                        break;
                    }
                    if (controller.nextRouterCalled()) {
                        LOG(TRACE) << "Express: R - NextRouter called - breaking dispatching";
                        break;
                    }
                }

                controller.setCaseInsensitiveRouting(oldCaseInsensitiveRouting);
                controller.setStrictRouting(oldStrictRouting);
            }
        } else {
            LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()
                       << " HTTP Express: router -> next(...) called";
            LOG(TRACE) << "           RequestMethod: " << controller.getRequest()->method;
            LOG(TRACE) << "              RequestUrl: " << controller.getRequest()->url;
            LOG(TRACE) << "             RequestPath: " << controller.getRequest()->path;
            LOG(TRACE) << "       AbsoluteMountPath: " << absoluteMountPath;
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
