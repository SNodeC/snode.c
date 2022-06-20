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

#include "Route.h"

#include "express/dispatcher/Dispatcher.h"
#include "express/dispatcher/RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    Route::Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher)
        : state(this)
        , mountPoint(method, relativeMountPath)
        , dispatcher(dispatcher) {
    }

    Route::Route()
        : state(this)
        , mountPoint("use", "/")
        , dispatcher(std::make_shared<express::dispatcher::RouterDispatcher>()) {
    }

    Route::Route(const Route& route)
        : state(this)
        , mountPoint(route.mountPoint)
        , dispatcher(route.dispatcher) {
    }

    Route::Route(Route&& route)
        : state(this)
        , mountPoint(std::move(route.mountPoint))
        , dispatcher(std::move(route.dispatcher)) {
    }

    Route& Route::operator=(const Route& route) {
        dispatcher = route.dispatcher;
        mountPoint = route.mountPoint;

        return *this;
    }

    Route& Route::operator=(Route&& route) {
        std::swap(dispatcher, route.dispatcher);
        std::swap(mountPoint, route.mountPoint);

        return *this;
    }

    std::shared_ptr<Dispatcher>& Route::getDispatcher() {
        return dispatcher;
    }

    bool Route::dispatch(State& state, const std::string& parentMountPath, Request& req, Response& res) const {
        return dispatcher->dispatch(state, parentMountPath, mountPoint, req, res);
    }

    bool Route::dispatch(Request& req, Response& res) {
        VLOG(0) << "Recurse: " << state.rootRoute;
        state.found = false;
        state.request = &req;
        state.response = &res;
        state.flags = State::NON;

        return dispatch(state, "/", req, res);
    }

    bool Route::dispatch(State& state) {
        VLOG(0) << "ReRecurs: RootRoute = " << state.rootRoute << ", LastRoute = " << state.lastRoute;

        std::swap(state.lastRoute, state.currentRoute);

        state.currentRoute = nullptr;
        return dispatch(state, "/", *state.request, *state.response);
    }

} // namespace express::dispatcher
