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

    bool RouterDispatcher::dispatch(express::Controller& controller, //
                                    const express::MountPoint& mountPoint,
                                    [[maybe_unused]] bool strictRoutingUnused,
                                    [[maybe_unused]] bool caseInsensitiveRoutingUnused,
                                    [[maybe_unused]] bool mergeParamsUnused) {
        bool dispatched = false;

        const bool methodMatchesResult = methodMatches(controller.getRequest()->method, mountPoint.method);

        LOG(TRACE) << "========================= ROUTER      DISPATCH =========================";
        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
        LOG(TRACE) << "           StrictRouting: " << strictRouting;
        LOG(TRACE) << "  CaseInsensitiveRouting: " << caseInsensitiveRouting;
        LOG(TRACE) << "             MergeParams: " << mergeParams;

        if (methodMatchesResult && (controller.getFlags() & Controller::NEXT) == 0) {
            const MountMatchResult match =
                matchMountPoint(controller, mountPoint.relativeMountPath, mountPoint, this->strictRouting, this->caseInsensitiveRouting);

            if (match.requestMatched) {
                LOG(TRACE) << "------------------------- ROUTER         MATCH -------------------------";

                dispatched = true;

                if (!match.decodeError) {
                    auto& req = *controller.getRequest();
                    req.queries.insert(match.requestQueryPairs.begin(), match.requestQueryPairs.end());

                    // Express-style mount path stripping is only applied for use()
                    const ScopedPathStrip pathStrip(req, req.url, match.isPrefix, match.consumedLength);
                    const ScopedParams scopedParams(req, match.params, this->mergeParams);

                    for (Route& route : routes) {
                        dispatched = route.dispatch(controller, this->strictRouting, this->caseInsensitiveRouting, this->mergeParams);

                        if (dispatched || controller.nextRouterCalled()) {
                            break;
                        }
                    }
                } else {
                    controller.getResponse()->sendStatus(400);
                }
            } else {
                LOG(TRACE) << "------------------------- ROUTER       NOMATCH -------------------------";
            }
        } else {
            LOG(TRACE) << "------------------------- ROUTER       NOMATCH -------------------------";
        }

        return dispatched;
    }

    std::list<std::string> RouterDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint) const {
        return getRoutes(parentMountPath, mountPoint, false);
    }

    std::list<std::string>
    RouterDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const {
        std::list<std::string> collectedRoutes;

        for (const Route& route : routes) {
            collectedRoutes.splice(collectedRoutes.end(),
                                   route.getRoute(parentMountPath + "$" + mountPoint.relativeMountPath + "$", strictRouting));
        }

        return collectedRoutes;
    }

    RouterDispatcher& RouterDispatcher::setStrictRouting(bool strictRouting) {
        this->strictRouting = strictRouting;

        return *this;
    }

    bool RouterDispatcher::getStrictRouting() const {
        return strictRouting;
    }

    RouterDispatcher& RouterDispatcher::setCaseInsensitiveRouting(bool caseInsensitiveRouting) {
        this->caseInsensitiveRouting = caseInsensitiveRouting;

        return *this;
    }

    bool RouterDispatcher::getCaseInsensitiveRouting() const {
        return caseInsensitiveRouting;
    }

    RouterDispatcher& RouterDispatcher::setMergeParams(bool mergeParams) {
        this->mergeParams = mergeParams;

        return *this;
    }

    bool RouterDispatcher::getMergeParams() const {
        return mergeParams;
    }

} // namespace express::dispatcher
