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

#ifndef EXPRESS_ROUTER_H
#define EXPRESS_ROUTER_H

#include "express/Request.h"          // IWYU pragma: export
#include "express/Response.h"         // IWYU pragma: export
#include "express/dispatcher/State.h" // IWYU pragma: export

namespace express {

    namespace dispatcher {

        class RouterDispatcher;
        class Route;

    } // namespace dispatcher

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MIDDLEWARE(req, res, state)                                                                                                        \
    ([[maybe_unused]] express::Request & (req),                                                                                            \
     [[maybe_unused]] express::Response & (res),                                                                                           \
     [[maybe_unused]] express::dispatcher::State(state))

#define APPLICATION(req, res) ([[maybe_unused]] express::Request & (req), [[maybe_unused]] express::Response & (res))

namespace express {

    class Router /*: protected express::dispatcher::Route*/ {
    public:
        Router();
        /*
                Router(const Router& router);
                Router(Router&& router);
                Router& operator=(const Router& router);
                Router& operator=(Router&& router);
        */

#define DECLARE_REQUESTMETHOD(METHOD)                                                                                                      \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& relativeMountPath, const Router& router);                                                            \
    Router& METHOD(const std::function<void(Request & req, Response & res)>& lambda);                                                      \
    Router& METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res)>& lambda);                \
    Router& METHOD(const std::function<void(Request & req, Response & res, express::dispatcher::State & state)>& lambda);                  \
    Router& METHOD(const std::string& relativeMountPath,                                                                                   \
                   const std::function<void(Request & req, Response & res, express::dispatcher::State & state)>& lambda);

        DECLARE_REQUESTMETHOD(use)
        DECLARE_REQUESTMETHOD(all)
        DECLARE_REQUESTMETHOD(get)
        DECLARE_REQUESTMETHOD(put)
        DECLARE_REQUESTMETHOD(post)
        DECLARE_REQUESTMETHOD(del)
        DECLARE_REQUESTMETHOD(connect)
        DECLARE_REQUESTMETHOD(options)
        DECLARE_REQUESTMETHOD(trace)
        DECLARE_REQUESTMETHOD(patch)
        DECLARE_REQUESTMETHOD(head)

    protected:
        std::shared_ptr<express::dispatcher::Route> route;
    };

} // namespace express

#endif // EXPRESS_ROUTER_H
