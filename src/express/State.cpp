/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "express/State.h"

#include "core/EventLoop.h"
#include "express/Request.h"
#include "express/RootRoute.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    State::State(Request& request, Response& response)
        : lastTick(core::EventLoop::getTickCounter())
        , request(&request)
        , response(&response) {
        request.extend();
    }

    void State::setRootRoute(RootRoute* rootRoute) {
        this->rootRoute = rootRoute;
    }

    void State::setCurrentRoute(Route* currentRoute) {
        this->currentRoute = currentRoute;
    }

    Request* State::getRequest() const {
        return request;
    }

    Response* State::getResponse() const {
        return response;
    }

    int State::getFlags() const {
        return flags;
    }

    void State::next(const std::string& how) {
        flags = NEXT;

        if (how == "route") {
            flags |= NEXT_ROUTE;
        } else if (how == "router") {
            flags |= NEXT_ROUTER;
        }

        lastRoute = currentRoute;

        if (lastTick != core::EventLoop::getTickCounter()) { // If asynchron next() start traversing of rout-tree
            rootRoute->dispatch(*this);
        }
    }

    bool State::nextRouter() {
        bool breakDispatching = false;

        if (lastRoute == currentRoute && (flags & State::NEXT_ROUTER) != 0) {
            flags &= ~State::NEXT_ROUTER;
            breakDispatching = true;
        }

        return breakDispatching;
    }

    bool State::dispatchNext(const std::string& parentMountPath) {
        bool dispatched = false;

        if (((flags & State::NEXT) != 0)) {
            if (lastRoute == currentRoute) {
                flags &= ~State::NEXT;
                if ((flags & State::NEXT_ROUTE) != 0) {
                    flags &= ~State::NEXT_ROUTE;
                } else if ((flags & State::NEXT_ROUTER) == 0) {
                    dispatched = currentRoute->dispatchNext(*this, parentMountPath);
                }
            } else { // ? Optimization: Dispatch only parent route matched path
                dispatched = currentRoute->dispatchNext(*this, parentMountPath);
            }
        }

        return dispatched;
    }

} // namespace express
