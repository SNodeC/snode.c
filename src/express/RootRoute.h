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

    class Dispatcher;
    class Request;
    class Response;
    class State;

    namespace dispatcher {

        class RouterDispatcher;

    }

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class RootRoute : protected express::Route {
    public:
        RootRoute() = default;
        RootRoute(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher);

        RootRoute(const RootRoute& route);
        RootRoute(RootRoute&& route);

        void dispatch(Request& req, Response& res);
        void dispatch(State& state);
        void dispatch(State&& state);

        std::shared_ptr<dispatcher::RouterDispatcher> getDispatcher();
        std::list<Route>& routes();
    };

} // namespace express

#endif // EXPRESS_ROOTROUTE_H
