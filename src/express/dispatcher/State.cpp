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

#include "MountPoint.h"
#include "RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    State::State(RouterDispatcher* routerDispatcher)
        : currentRouterDispatcher(routerDispatcher) {
    }

    State::~State() {
        if (parentState != nullptr) {
            delete parentState;
        }
    }

    State::State(const State& state) {
        proceed = state.proceed;
        parentProceed = state.parentProceed;
        currentRouterDispatcher = state.currentRouterDispatcher;
        parentRouterDispatcher = state.parentRouterDispatcher;
        currentRoute = state.currentRoute;
        request = state.request;
        response = state.response;
        absoluteMountPath = state.absoluteMountPath;
        mountPoint = state.mountPoint;
        if (state.parentState != nullptr) {
            parentState = new State(*state.parentState);
        }

        /*
                bool proceed = true;
                bool parentProceed = false;

                RouterDispatcher* currentRouterDispatcher = nullptr;
                RouterDispatcher* parentRouterDispatcher = nullptr;

                const Route* currentRoute = nullptr;
                Request* request = nullptr;
                Response* response = nullptr;
                std::string absoluteMountPath;
                const MountPoint* mountPoint = nullptr;

                State* parentState = nullptr;
        */
    }

    void State::operator()(const std::string& how) {
        if (how == "route") {
            //            parentProceed = true;
        } else {
            //            proceed = true;
        }

        if (how == "route") {
            parentRouterDispatcher->dispatchContinue(*parentState);
        } else {
            currentRouterDispatcher->dispatchContinue(*this);
        }
    }

    void State::set(RouterDispatcher* parentRouterDispatcher,
                    const std::string& absoluteMountPath,
                    const MountPoint& mountPoint,
                    Request& req,
                    Response& res) {
        this->parentRouterDispatcher = parentRouterDispatcher;
        if (parentRouterDispatcher != nullptr) {
            if (this->parentState != nullptr) {
                delete this->parentState;
            }
            this->parentState = new State(parentRouterDispatcher->getState());
        }
        request = &req;
        response = &res;
        this->absoluteMountPath = absoluteMountPath;
        this->mountPoint = &mountPoint;
    }

    void State::set(Route& route) {
        currentRoute = &route;
    }

} // namespace express::dispatcher
