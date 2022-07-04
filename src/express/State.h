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
    class RootRoute;
    class Route;

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class State {
    public:
        State(Request& request, Response& response);

        void setRootRoute(RootRoute* rootRoute);
        void setCurrentRoute(Route* newCurrentRoute);

        void switchRoutes();

        int getFlags() const;

        Request* getRequest() const;
        Response* getResponse() const;

        void next(const std::string& how);
        bool next(Route& route);

        enum Flags { NON = 0, INH = 1 << 0, NXT = 1 << 1 };

        RootRoute* rootRoute = nullptr;

        Route* lastRoute = nullptr;
        Route* currentRoute = nullptr;

        Request* request = nullptr;
        Response* response = nullptr;

        int flags = NON;
    };

} // namespace express

#endif // EXPRESS_DISPATCHER_STATE_H
