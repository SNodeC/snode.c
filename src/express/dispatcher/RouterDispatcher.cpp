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

#include "RouterDispatcher.h"

#include "express/Request.h" // for Request
#include "express/dispatcher/MountPoint.h"
#include "express/dispatcher/Route.h"
#include "express/dispatcher/regex_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for find_if

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    RouterDispatcher::RouterDispatcher()
        : state(this) {
    }

    void RouterDispatcher::dispatch(Request& req, Response& res) {
        dispatch(nullptr, "/", MountPoint("use", "/"), req, res);
    }

    bool RouterDispatcher::dispatch(
        RouterDispatcher* parentRouter, const std::string& parentMountPath, const MountPoint& mountPoint, Request& req, Response& res) {
        bool catched = false;

        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        // TODO: Fix regex-match
        if ((req.path.rfind(absoluteMountPath, 0) == 0 &&
             (mountPoint.method == "use" || req.method == mountPoint.method || mountPoint.method == "all"))) {
            State* parentState = (parentRouter != nullptr) ? &parentRouter->getState() : nullptr;
            state.set(parentRouter, parentState, absoluteMountPath, req, res);

            for (Route& route : routes) {
                state.set(route);
                catched = route.dispatch(absoluteMountPath, req, res);

                if (catched) {
                    break;
                }
            }
        }

        return catched;
    }

    void RouterDispatcher::dispatchContinue(State& state) {
        std::list<Route>::iterator routeBegin = std::find_if(routes.begin(), routes.end(), [&state](const Route& route) -> bool {
            return &route == state.currentRoute;
        });
        ++routeBegin;

        this->state.set(state.parentRouterDispatcher, state.parentState, state.absoluteMountPath, *state.request, *state.response);

        for (std::list<Route>::iterator route = routeBegin; route != routes.end(); ++route) {
            this->state.set(*route);

            if (route->dispatch(state.absoluteMountPath, *state.request, *state.response)) {
                break;
            }
        }
    }

    State& RouterDispatcher::getState() {
        return state;
    }

} // namespace express::dispatcher
