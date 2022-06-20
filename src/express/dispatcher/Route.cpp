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

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    Route::Route(RouterDispatcher* parentRouter,
                 const std::string& method,
                 const std::string& relativeMountPath,
                 const std::shared_ptr<Dispatcher>& dispatcher)
        : parentRouter(parentRouter)
        , mountPoint(method, relativeMountPath)
        , dispatcher(dispatcher) {
    }

    bool Route::dispatch(const std::string& parentMountPath, Request& req, Response& res) const {
        return dispatcher->dispatch(parentRouter, parentMountPath, mountPoint, req, res);
    }

    Route::Route()
        : parentRouter(nullptr)
        , mountPoint("use", "/")
        , dispatcher(std::make_shared<express::dispatcher::RouterDispatcher>()) {
    }

    Route::Route(const Route& route)
        : parentRouter(route.parentRouter)
        , mountPoint(route.mountPoint)
        , dispatcher(route.dispatcher) {
    }

    Route::Route(Route&& route)
        : parentRouter(std::move(route.parentRouter))
        , mountPoint(std::move(route.mountPoint))
        , dispatcher(std::move(route.dispatcher)) {
    }

    Route& Route::operator=(const Route& route) {
        parentRouter = route.parentRouter;
        dispatcher = route.dispatcher;
        mountPoint = route.mountPoint;

        return *this;
    }

    Route& Route::operator=(Route&& route) {
        std::swap(parentRouter, route.parentRouter);
        std::swap(dispatcher, route.dispatcher);
        std::swap(mountPoint, route.mountPoint);

        return *this;
    }

    bool Route::dispatch(Request& req, Response& res) {
        return dispatch("/", req, res);
    }

    Dispatcher* Route::getDispatcher() {
        return dispatcher.get();
    }

} // namespace express::dispatcher
