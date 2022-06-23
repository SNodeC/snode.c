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

#ifndef EXPRESS_DISPATCHER_STATE_H
#define EXPRESS_DISPATCHER_STATE_H

namespace express {

    class Request;
    class Response;

    namespace dispatcher {

        class Route;
        class RootRoute;

    } // namespace dispatcher

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    class State {
    private:
        explicit State(RootRoute* rootRoute);

    public:
        void operator()(const std::string& how = "") const;

    private:
        express::Request* request = nullptr;
        express::Response* response = nullptr;

        RootRoute* rootRoute = nullptr;
        Route* lastRoute = nullptr;
        Route* currentRoute = nullptr;

        enum { NON = 0, INH = 1 << 0, NXT = 1 << 1 };
        mutable int flags;

        friend class RootRoute;
        friend class RouterDispatcher;
        friend class ApplicationDispatcher;
        friend class MiddlewareDispatcher;
    };

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_STATE_H
