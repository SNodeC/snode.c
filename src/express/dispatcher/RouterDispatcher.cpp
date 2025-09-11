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

#include "express/Controller.h"
#include "express/Request.h"
#include "express/Route.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <memory>

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

    bool
    RouterDispatcher::dispatch(express::Controller& controller, const std::string& parentMountPath, const express::MountPoint& mountPoint) {
        bool dispatched = false;

        const std::string absoluteMountPath = parentMountPath + mountPoint.relativeMountPath;

        const std::string requestMethod = controller.getRequest()->method;
        const std::string requestUrl = controller.getRequest()->url;
        const std::string requestPath = controller.getRequest()->path;

        // clang-format off
        const bool requestMatched =
            (
                (
                    (
                        mountPoint.method == controller.getRequest()->method
                    ) || (
                        mountPoint.method == "all"
                    ) || (
                        mountPoint.method == "use"
                    )
                ) && (
                    (
                        requestUrl.starts_with(absoluteMountPath)
                    )
                )
            );
        // clang-format on
        LOG(TRACE) << "Express: router";
        LOG(TRACE) << "      RequestMethod: " << controller.getRequest()->method;
        LOG(TRACE) << "         RequestUrl: " << controller.getRequest()->url;
        LOG(TRACE) << "        RequestPath: " << controller.getRequest()->path;
        LOG(TRACE) << "  AbsoluteMountPath: " << absoluteMountPath;
        LOG(TRACE) << "      StrictRouting: " << controller.getStrictRouting();
        LOG(TRACE) << "      StrictRouting: " << controller.getStrictRouting();
        LOG(TRACE) << "              MATCH: " << (requestMatched ? "true" : "false");

        if (requestMatched) {
            LOG(TRACE) << "      MATCH";
            for (Route& route : routes) {
                const bool oldStrictRouting = controller.setStrictRouting(strictRouting);

                dispatched = route.dispatch(controller, absoluteMountPath);

                controller.setStrictRouting(oldStrictRouting);

                if (dispatched) {
                    LOG(TRACE) << "Express: R - Dispatched: " << dispatched;
                }
                if (controller.nextRouterCalled()) {
                    LOG(TRACE) << "Express: R - NextRouter called - breaking dispatching";
                }
                if (dispatched || controller.nextRouterCalled()) {
                    break;
                }
            }
        } else {
            LOG(TRACE) << "      NO MQTCH";
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
