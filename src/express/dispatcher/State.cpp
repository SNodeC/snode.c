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

#include "State.h"

#include "RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    State::State(RouterDispatcher* routerDispatcher)
        : currentRouterDispatcher(routerDispatcher)
        , parentState(new State()) {
    }

    State::State(const State& state)
        : currentRouterDispatcher(state.currentRouterDispatcher)
        , parentRouterDispatcher(state.parentRouterDispatcher)
        , currentRoute(state.currentRoute)
        , request(state.request)
        , response(state.response)
        , absoluteMountPath(state.absoluteMountPath)
        , parentState(new State()) {
        if (state.parentState != nullptr) {
            *parentState = *state.parentState;
        }
    }

    State::~State() {
        if (parentState != nullptr) {
            delete parentState;
        }
    }

    State& State::operator=(const State& state) {
        if (this != &state) {
            currentRouterDispatcher = state.currentRouterDispatcher;
            parentRouterDispatcher = state.parentRouterDispatcher;
            currentRoute = state.currentRoute;
            request = state.request;
            response = state.response;
            absoluteMountPath = state.absoluteMountPath;
            if (parentState != nullptr) {
                *parentState = *state.parentState;
            }
        }

        return *this;
    }

    void State::set(RouterDispatcher* parentRouterDispatcher,
                    const State* parentState,
                    const std::string& absoluteMountPath,
                    Request& req,
                    Response& res) {
        this->parentRouterDispatcher = parentRouterDispatcher;
        if (parentState != nullptr) {
            *this->parentState = *parentState;
        }
        request = &req;
        response = &res;
        this->absoluteMountPath = absoluteMountPath;
    }

    void State::set(Route& route) {
        currentRoute = &route;
    }

    void State::operator()(const std::string& how) const {
        if (how == "route") {
            parentRouterDispatcher->dispatchContinue(*parentState);
        } else {
            currentRouterDispatcher->dispatchContinue(*this);
        }
    }

} // namespace express::dispatcher
