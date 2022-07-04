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

#include "express/RootRoute.h"

#include "express/Response.h"
#include "express/dispatcher/RouterDispatcher.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    RootRoute::RootRoute()
        : Route()
        , state(this) {
    }

    RootRoute::RootRoute(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher)
        : Route(method, relativeMountPath, dispatcher)
        , state(this) {
    }

    RootRoute::RootRoute(const RootRoute& route)
        : Route(route)
        , state(this) {
    }

    RootRoute::RootRoute(RootRoute&& route)
        : Route(route)
        , state(this) {
    }

    std::shared_ptr<express::dispatcher::RouterDispatcher> RootRoute::getDispatcher() {
        return std::dynamic_pointer_cast<express::dispatcher::RouterDispatcher>(dispatcher);
    }

    std::list<Route>& RootRoute::routes() {
        return std::dynamic_pointer_cast<express::dispatcher::RouterDispatcher>(dispatcher)->routes;
    }

    void RootRoute::dispatch(Request& req, Response& res) {
        state.lastRoute = nullptr;
        state.currentRoute = nullptr;

        state.request = &req;
        state.response = &res;

        Route::dispatch(state);
    }

    void RootRoute::dispatch(State& state) {
        state.lastRoute = state.currentRoute;
        state.currentRoute = nullptr;

        Route::dispatch(state);
    }

} // namespace express
