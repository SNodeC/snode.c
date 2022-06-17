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
        class RouterDispatcher;
    } // namespace dispatcher
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    class State {
    public:
        State() = default;
        explicit State(RouterDispatcher* routerDispatcher);
        State(const State& state);

        ~State();

        State& operator=(const State& state);

        void set(RouterDispatcher* parentRouterDispatcher,
                 const State* parentState,
                 const std::string& absoluteMountPath,
                 Request& req,
                 Response& res);

        void set(Route& route);

        void operator()(const std::string& how = "");

    private:
        RouterDispatcher* currentRouterDispatcher = nullptr;
        RouterDispatcher* parentRouterDispatcher = nullptr;

        const Route* currentRoute = nullptr;
        Request* request = nullptr;
        Response* response = nullptr;
        std::string absoluteMountPath;

        State* parentState = nullptr;

        friend class RouterDispatcher;
    };

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_STATE_H
