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

#include "express/Router.h"

#include "express/dispatcher/ApplicationDispatcher.h"
#include "express/dispatcher/MiddlewareDispatcher.h"
#include "express/dispatcher/Route.h"
#include "express/dispatcher/RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <vector> // IWYU pragma: keep

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Router::Router() {
    }
/*
    Router::Router(const Router& router)
        : express::dispatcher::Route(router) {
    }

    Router::Router(Router&& router)
        : express::dispatcher::Route(router) {
    }

    Router& Router::operator=(const Router& router) {
        express::dispatcher::Route::operator=(router);

        return *this;
    }

    Router& Router::operator=(Router&& router) {
        express::dispatcher::Route::operator=(router);

        return *this;
    }
*/
#define DEFINE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                          \
    Router& Router::METHOD(const std::string& relativeMountPath, const Router& router) {                                                   \
        static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get())                                                              \
            ->routes.emplace_back(express::dispatcher::Route(static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get()),        \
                                                             HTTP_METHOD,                                                                  \
                                                             relativeMountPath,                                                            \
                                                             router.dispatcher));                                                          \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const Router& router) {                                                                                         \
        static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get())                                                              \
            ->routes.emplace_back(express::dispatcher::Route(                                                                              \
                static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get()), HTTP_METHOD, "/", router.dispatcher));              \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::string& relativeMountPath,                                                                           \
                           const std::function<void(Request & req, Response & res, express::dispatcher::State & state)>& lambda) {         \
        static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get())                                                              \
            ->routes.emplace_back(express::dispatcher::Route(static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get()),        \
                                                             HTTP_METHOD,                                                                  \
                                                             relativeMountPath,                                                            \
                                                             std::make_shared<express::dispatcher::MiddlewareDispatcher>(lambda)));        \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res, express::dispatcher::State & state)>& lambda) {         \
        static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get())                                                              \
            ->routes.emplace_back(express::dispatcher::Route(static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get()),        \
                                                             HTTP_METHOD,                                                                  \
                                                             "/",                                                                          \
                                                             std::make_shared<express::dispatcher::MiddlewareDispatcher>(lambda)));        \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res)>& lambda) {       \
        static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get())                                                              \
            ->routes.emplace_back(express::dispatcher::Route(static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get()),        \
                                                             HTTP_METHOD,                                                                  \
                                                             relativeMountPath,                                                            \
                                                             std::make_shared<express::dispatcher::ApplicationDispatcher>(lambda)));       \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res)>& lambda) {                                             \
        static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get())                                                              \
            ->routes.emplace_back(express::dispatcher::Route(static_cast<express::dispatcher::RouterDispatcher*>(dispatcher.get()),        \
                                                             HTTP_METHOD,                                                                  \
                                                             "/",                                                                          \
                                                             std::make_shared<express::dispatcher::ApplicationDispatcher>(lambda)));       \
        return *this;                                                                                                                      \
    }

    DEFINE_REQUESTMETHOD(use, "use")
    DEFINE_REQUESTMETHOD(all, "all")
    DEFINE_REQUESTMETHOD(get, "GET")
    DEFINE_REQUESTMETHOD(put, "PUT")
    DEFINE_REQUESTMETHOD(post, "POST")
    DEFINE_REQUESTMETHOD(del, "DELETE")
    DEFINE_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_REQUESTMETHOD(trace, "TRACE")
    DEFINE_REQUESTMETHOD(patch, "PATCH")
    DEFINE_REQUESTMETHOD(head, "HEAD")

} // namespace express
