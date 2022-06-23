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

#ifndef EXPRESS_DISPATCHER_ROUTERDISPATCHER_H
#define EXPRESS_DISPATCHER_ROUTERDISPATCHER_H

#include "express/dispatcher/Dispatcher.h" // IWYU pragma: export
#include "express/dispatcher/Route.h"      // for Route

namespace express {

    class Router;

    namespace dispatcher {

        class State;

    } // namespace dispatcher

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    class RouterDispatcher : public Dispatcher {
    private:
        bool dispatch(State& state, const std::string& parentMountPath, const MountPoint& mountPoint) override;

        std::list<Route> routes;

        friend class express::Router;
        friend class ApplicationDispatcher;
        friend class MiddlewareDispatcher;
        friend class RootRoute;
    };

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_ROUTERDISPATCHER_H
