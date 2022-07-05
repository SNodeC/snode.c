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

#include "express/Request.h"
#include "express/RootRoute.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    State::State(Request& request, Response& response)
        : request(&request)
        , response(&response) {
        request.extend();
    }

    void State::setRootRoute(RootRoute* rootRoute) {
        this->rootRoute = rootRoute;
    }

    void State::setCurrentRoute(Route* currentRoute) {
        this->currentRoute = currentRoute;
    }

    void State::switchRoutes() {
        lastRoute = currentRoute;
        currentRoute = nullptr;
    }

    int State::getFlags() const {
        return flags;
    }

    express::Request* State::getRequest() const {
        return request;
    }

    express::Response* State::getResponse() const {
        return response;
    }

    void State::next(const std::string& how) {
        flags |= INH;

        if (how == "route") {
            flags |= NXT;
        }

        rootRoute->dispatch(*const_cast<express::State*>(this));
    }

    bool State::next(Route& route) {
        bool breakDispatching = false;

        if (lastRoute == &route && (flags & State::NXT) != 0) {
            flags &= ~State::NXT;
            breakDispatching = true;
        }

        return breakDispatching;
    }

    bool State::next(std::shared_ptr<Route>& nextRoute, const std::string& parentMountPath) {
        bool dispatched = false;

        if (lastRoute == currentRoute && (flags & State::INH) != 0) {
            flags &= ~State::INH;
        }

        if (nextRoute != nullptr && (flags & State::NXT) == 0) {
            dispatched = nextRoute->dispatch(*const_cast<express::State*>(this), parentMountPath);
        }

        return dispatched;
    }

} // namespace express
