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

#include "express/Route.h"

#include "express/Router.h"
#include "express/dispatcher/ApplicationDispatcher.h"
#include "express/dispatcher/MiddlewareDispatcher.h"
#include "express/dispatcher/RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DEFINE_ROUTE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                    \
    Route& Route::METHOD(const Route& route) {                                                                                             \
        return *(nextRoute = std::make_shared<express::Route>(HTTP_METHOD, "", route.dispatcher)).get();                                   \
    }                                                                                                                                      \
    Route& Route::METHOD(const RootRoute& rootRoute) {                                                                                     \
        return *(nextRoute = std::make_shared<express::Route>(HTTP_METHOD, "", rootRoute.getDispatcher())).get();                          \
    }                                                                                                                                      \
    Route& Route::METHOD(const std::function<void(Request & req, Response & res, express::Next && state)>& lambda) {                       \
        return *(nextRoute = std::make_shared<express::Route>(                                                                             \
                     HTTP_METHOD, "", std::make_shared<express::dispatcher::MiddlewareDispatcher>(lambda)))                                \
                    .get();                                                                                                                \
    }                                                                                                                                      \
    Route& Route::METHOD(const std::function<void(Request & req, Response & res)>& lambda) {                                               \
        return *(nextRoute = std::make_shared<express::Route>(                                                                             \
                     HTTP_METHOD, "", std::make_shared<express::dispatcher::ApplicationDispatcher>(lambda)))                               \
                    .get();                                                                                                                \
    }

namespace express {

    Route::Route()
        : mountPoint("use", "/")
        , dispatcher(std::make_shared<express::dispatcher::RouterDispatcher>()) {
    }

    Route::Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher)
        : mountPoint(method, relativeMountPath)
        , dispatcher(dispatcher) {
    }

    bool Route::dispatch(State& state, const std::string& parentMountPath) {
        state.setCurrentRoute(this);

        bool dispatched = dispatcher->dispatch(state, parentMountPath, mountPoint);

        if (!dispatched) {
            dispatched = state.next(nextRoute, parentMountPath);
        }

        return dispatched;
    }

    bool Route::dispatch(State& state) {
        return dispatch(state, "");
    }

    DEFINE_ROUTE_REQUESTMETHOD(use, "use")
    DEFINE_ROUTE_REQUESTMETHOD(all, "all")
    DEFINE_ROUTE_REQUESTMETHOD(get, "GET")
    DEFINE_ROUTE_REQUESTMETHOD(put, "PUT")
    DEFINE_ROUTE_REQUESTMETHOD(post, "POST")
    DEFINE_ROUTE_REQUESTMETHOD(del, "DELETE")
    DEFINE_ROUTE_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_ROUTE_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_ROUTE_REQUESTMETHOD(trace, "TRACE")
    DEFINE_ROUTE_REQUESTMETHOD(patch, "PATCH")
    DEFINE_ROUTE_REQUESTMETHOD(head, "HEAD")

} // namespace express
