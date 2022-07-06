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
#include "express/State.h"
#include "express/dispatcher/ApplicationDispatcher.h"
#include "express/dispatcher/MiddlewareDispatcher.h"
#include "express/dispatcher/RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DEFINE_ROOTROUTE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                \
    Route& RootRoute::METHOD(const RootRoute& rootRoute) {                                                                                 \
        return routes().emplace_back(Route(HTTP_METHOD, "/", rootRoute.getDispatcher()));                                                  \
    }                                                                                                                                      \
    Route& RootRoute::METHOD(const std::string& relativeMountPath, const RootRoute& rootRoute) {                                           \
        return routes().emplace_back(Route(HTTP_METHOD, relativeMountPath, rootRoute.getDispatcher()));                                    \
    }                                                                                                                                      \
    Route& RootRoute::METHOD(const std::string& relativeMountPath,                                                                         \
                             const std::function<void(Request & req, Response & res, Next && state)>& lambda) {                            \
        return routes().emplace_back(Route(HTTP_METHOD, relativeMountPath, std::make_shared<dispatcher::MiddlewareDispatcher>(lambda)));   \
    }                                                                                                                                      \
    Route& RootRoute::METHOD(const std::function<void(Request & req, Response & res, Next && state)>& lambda) {                            \
        return routes().emplace_back(Route(HTTP_METHOD, "/", std::make_shared<dispatcher::MiddlewareDispatcher>(lambda)));                 \
    }                                                                                                                                      \
    Route& RootRoute::METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res)>& lambda) {     \
        return routes().emplace_back(Route(HTTP_METHOD, relativeMountPath, std::make_shared<dispatcher::ApplicationDispatcher>(lambda)));  \
    }                                                                                                                                      \
    Route& RootRoute::METHOD(const std::function<void(Request & req, Response & res)>& lambda) {                                           \
        return routes().emplace_back(Route(HTTP_METHOD, "/", std::make_shared<dispatcher::ApplicationDispatcher>(lambda)));                \
    }

namespace express {

    std::shared_ptr<dispatcher::RouterDispatcher> RootRoute::getDispatcher() const {
        return std::dynamic_pointer_cast<dispatcher::RouterDispatcher>(dispatcher);
    }

    std::list<Route>& RootRoute::routes() {
        return std::dynamic_pointer_cast<dispatcher::RouterDispatcher>(dispatcher)->getRoutes();
    }

    void RootRoute::dispatch(Request& req, Response& res) {
        dispatch(State(req, res));
    }

    void RootRoute::dispatch(State&& state) {
        dispatch(state);
    }

    void RootRoute::dispatch(State& state) {
        state.setRootRoute(this);
        state.switchRoutes();

        if (!Route::dispatch(state, "")) {
            state.getResponse()->sendStatus(501);
        }
    }

    DEFINE_ROOTROUTE_REQUESTMETHOD(use, "use")
    DEFINE_ROOTROUTE_REQUESTMETHOD(all, "all")
    DEFINE_ROOTROUTE_REQUESTMETHOD(get, "GET")
    DEFINE_ROOTROUTE_REQUESTMETHOD(put, "PUT")
    DEFINE_ROOTROUTE_REQUESTMETHOD(post, "POST")
    DEFINE_ROOTROUTE_REQUESTMETHOD(del, "DELETE")
    DEFINE_ROOTROUTE_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_ROOTROUTE_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_ROOTROUTE_REQUESTMETHOD(trace, "TRACE")
    DEFINE_ROOTROUTE_REQUESTMETHOD(patch, "PATCH")
    DEFINE_ROOTROUTE_REQUESTMETHOD(head, "HEAD")

} // namespace express
