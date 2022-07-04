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

#ifndef EXPRESS_ROUTE_H
#define EXPRESS_ROUTE_H

#include "express/MountPoint.h" // IWYU pragma: export

namespace express {

    class State;
    class Dispatcher;
    class RootRoute;
    class Request;
    class Response;
    class Next;
    class Route;

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DECLARE_ROUTE_REQUESTMETHOD(METHOD)                                                                                                \
    Route& METHOD(const Route& router);                                                                                                    \
    Route& METHOD(const RootRoute& router);                                                                                                \
    Route& METHOD(const std::function<void(Request & req, Response & res)>& lambda);                                                       \
    Route& METHOD(const std::function<void(Request & req, Response & res, express::Next && state)>& lambda);

namespace express {

    class Route {
    public:
        Route();
        Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher);

        bool dispatch(express::State& state, const std::string& parentMountPath);

    protected:
        bool dispatch(express::State& state);

        MountPoint mountPoint;

        std::shared_ptr<Dispatcher> dispatcher;
        std::shared_ptr<Route> nextRoute = nullptr;

    public:
        DECLARE_ROUTE_REQUESTMETHOD(use)
        DECLARE_ROUTE_REQUESTMETHOD(all)
        DECLARE_ROUTE_REQUESTMETHOD(get)
        DECLARE_ROUTE_REQUESTMETHOD(put)
        DECLARE_ROUTE_REQUESTMETHOD(post)
        DECLARE_ROUTE_REQUESTMETHOD(del)
        DECLARE_ROUTE_REQUESTMETHOD(connect)
        DECLARE_ROUTE_REQUESTMETHOD(options)
        DECLARE_ROUTE_REQUESTMETHOD(trace)
        DECLARE_ROUTE_REQUESTMETHOD(patch)
        DECLARE_ROUTE_REQUESTMETHOD(head)
    };

} // namespace express

#endif // EXPRESS_ROUTE_H
