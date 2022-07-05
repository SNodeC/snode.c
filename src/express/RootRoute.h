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

#ifndef EXPRESS_ROOTROUTE_H
#define EXPRESS_ROOTROUTE_H

#include "express/Route.h" // IWYU pragma: export

namespace express {

    class Request;
    class Response;
    class Next;
    class State;
    class RootRoute;

    namespace dispatcher {

        class RouterDispatcher;

    }

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DECLARE_ROOTROUTE_REQUESTMETHOD(METHOD)                                                                                            \
    Route& METHOD(const RootRoute& rootRoute);                                                                                             \
    Route& METHOD(const std::string& relativeMountPath, const RootRoute& rootRoute);                                                       \
    Route& METHOD(const std::function<void(Request & req, Response & res)>& lambda);                                                       \
    Route& METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res)>& lambda);                 \
    Route& METHOD(const std::function<void(Request & req, Response & res, express::Next && state)>& lambda);                               \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(Request & req, Response & res, express::Next && state)>& lambda);

namespace express {

    class RootRoute : protected express::Route {
    public:
        RootRoute() = default;

        void dispatch(Request& req, Response& res);
        void dispatch(State&& state);
        void dispatch(State& state);

    protected:
        std::shared_ptr<dispatcher::RouterDispatcher> getDispatcher() const;
        std::list<Route>& routes();

    public:
        DECLARE_ROOTROUTE_REQUESTMETHOD(use)
        DECLARE_ROOTROUTE_REQUESTMETHOD(all)
        DECLARE_ROOTROUTE_REQUESTMETHOD(get)
        DECLARE_ROOTROUTE_REQUESTMETHOD(put)
        DECLARE_ROOTROUTE_REQUESTMETHOD(post)
        DECLARE_ROOTROUTE_REQUESTMETHOD(del)
        DECLARE_ROOTROUTE_REQUESTMETHOD(connect)
        DECLARE_ROOTROUTE_REQUESTMETHOD(options)
        DECLARE_ROOTROUTE_REQUESTMETHOD(trace)
        DECLARE_ROOTROUTE_REQUESTMETHOD(patch)
        DECLARE_ROOTROUTE_REQUESTMETHOD(head)

        friend class Route;
    };

} // namespace express

#endif // EXPRESS_ROOTROUTE_H
