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

#include <list>
#include <unordered_map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    ApplicationDispatcher::ApplicationDispatcher(
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda)
        : lambda(lambda) {
    }

    bool ApplicationDispatcher::dispatch(express::Controller& controller, //
                                         const express::MountPoint& mountPoint,
                                         bool strictRouting,
                                         bool caseInsensitiveRouting,
                                         bool mergeParams) {
        LOG(TRACE) << "======================= APPLICATION DISPATCH =======================";
        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
        LOG(TRACE) << "           StrictRouting: " << strictRouting;
        LOG(TRACE) << "  CaseInsensitiveRouting: " << caseInsensitiveRouting;
        LOG(TRACE) << "             MergeParams: " << mergeParams;

        bool dispatched = false;

        const bool methodMatchesResult = methodMatches(controller.getRequest()->method, mountPoint.method);

        if (methodMatchesResult && ((controller.getFlags() & Controller::NEXT) == 0)) {
            const MountMatchResult match =
                matchMountPoint(controller, mountPoint.relativeMountPath, mountPoint, regex, names, strictRouting, caseInsensitiveRouting);

            if (match.requestMatched) {
                LOG(TRACE) << "----------------------- APPLICATION    MATCH -----------------------";

                dispatched = true;

                if (!match.decodeError) {
                    express::Request& request = *controller.getRequest();
                    request.queries.insert(match.requestQueryPairs.begin(), match.requestQueryPairs.end());

                    // Express-style mount path stripping is only applied for use()
                    const ScopedPathStrip pathStrip(request, match.isPrefix, match.consumedLength);
                    const ScopedParams scopedParams(request, match.params, mergeParams);

                    lambda(controller.getRequest(), controller.getResponse());
                } else {
                    controller.getResponse()->sendStatus(400);
                }

            } else {
                LOG(TRACE) << "----------------------- APPLICATION  NOMATCH -----------------------";
            }
        } else {
            LOG(TRACE) << "----------------------- APPLICATION  NOMATCH -----------------------";
        }

        return dispatched;
    }

    std::list<std::string>
    ApplicationDispatcher::getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const {
        std::list<std::string> routes{"A " + joinMountPath(parentMountPath, mountPoint.relativeMountPath) + (!strictRouting ? "*" : "")};
        routes.push_back("  " + mountPoint.method + " " + mountPoint.relativeMountPath);

        if (nextRoute) {
            routes.splice(routes.end(), nextRoute->getRoute(parentMountPath, strictRouting));
        }

        return routes;
    }

} // namespace express::dispatcher
