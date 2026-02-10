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

#include "express/Controller.h"

#include "core/EventLoop.h"
#include "express/Request.h"
#include "express/Response.h"
#include "express/RootRoute.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Controller::Controller(const std::shared_ptr<web::http::server::Request>& request,
                           const std::shared_ptr<web::http::server::Response>& response)
        : request(std::make_shared<Request>(request))
        , response(std::make_shared<Response>(response))
        , lastTick(core::EventLoop::getTickCounter()) {
    }

    Controller::Controller(const Controller& controller)
        : request(controller.request)
        , response(controller.response)
        , rootRoute(controller.rootRoute)
        , lastRoute(controller.lastRoute)
        , currentRoute(controller.currentRoute)
        , lastTick(controller.lastTick)
        , flags(controller.flags) {
    }

    Controller& Controller::operator=(const Controller& controller) noexcept {
        if (this != &controller) {
            request = controller.request;
            response = controller.response;
            rootRoute = controller.rootRoute;
            lastRoute = controller.lastRoute;
            currentRoute = controller.currentRoute;
            lastTick = controller.lastTick;
            flags = controller.flags;
        }

        return *this;
    }

    void Controller::setRootRoute(RootRoute* rootRoute) {
        this->rootRoute = rootRoute;
    }

    void Controller::setCurrentRoute(Route* currentRoute) {
        this->currentRoute = currentRoute;
    }

    const std::shared_ptr<Request>& Controller::getRequest() {
        return request;
    }

    const std::shared_ptr<Response>& Controller::getResponse() {
        return response;
    }

    int Controller::getFlags() const {
        return flags;
    }

    void Controller::next(const std::string& how) const {
        flags = NEXT;

        if (how == "route") {
            flags |= NEXT_ROUTE;
        } else if (how == "router") {
            flags |= NEXT_ROUTER;
        }

        lastRoute = currentRoute;

        if (lastTick != core::EventLoop::getTickCounter()) { // If asynchronous next() start traversing of route-tree
            rootRoute->dispatch(*const_cast<express::Controller*>(this));
        }
    }

    bool Controller::nextRouterCalled() {
        const bool breakDispatching = lastRoute == currentRoute && (flags & Controller::NEXT_ROUTER) != 0;

        if (breakDispatching) {
            flags &= ~Controller::NEXT_ROUTER;
        }

        return breakDispatching;
    }

    bool Controller::dispatchNext(const std::string& parentMountPath,
                                  [[maybe_unused]] bool strictRouting,
                                  [[maybe_unused]] bool caseInsensitiveRouting,
                                  [[maybe_unused]] bool mergeParams) {
        bool dispatched = false;

        if (((flags & Controller::NEXT) != 0)) {
            if (lastRoute == currentRoute) {
                flags &= ~Controller::NEXT;
                if ((flags & Controller::NEXT_ROUTE) != 0) {
                    flags &= ~Controller::NEXT_ROUTE;
                } else if ((flags & Controller::NEXT_ROUTER) == 0) {
                    dispatched = currentRoute->dispatchNext(*this,
                                                            parentMountPath,
                                                            currentRoute->strictRouting,
                                                            currentRoute->caseInsensitiveRouting,
                                                            currentRoute->mergeParams);
                }
            } else { // ? Optimization: Dispatch only parent route matched path
                dispatched = currentRoute->dispatchNext(
                    *this, parentMountPath, currentRoute->strictRouting, currentRoute->caseInsensitiveRouting, currentRoute->mergeParams);
            }
        }

        return dispatched;
    }

} // namespace express
