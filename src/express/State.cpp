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

#include "express/State.h"

#include "express/RootRoute.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    State::State(RootRoute* rootRoute)
        : rootRoute(rootRoute) {
    }

    express::Request* State::getRequest() const {
        return request;
    }

    express::Response* State::getResponse() const {
        return response;
    }

    Route* State::getLastRoute1() const {
        return lastRoute;
    }

    int State::getFlags() const {
        return flags;
    }

    Next::Next(State& state)
        : state(state) {
    }

    void Next::operator()(const std::string& how) const {
        state.flags |= State::INH;

        if (how == "route") {
            state.flags |= State::NXT;
        }

        state.rootRoute->dispatch(*const_cast<express::State*>(&state));
    }

} // namespace express
