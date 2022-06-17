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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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

    void Route::dispatch(const std::string& parentMountPath, Request& req, Response& res) {
        return dispatcher->dispatch(parentRouter, parentMountPath, mountPoint, req, res);
    }

} // namespace express::dispatcher
