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

#include "express/Route.h"

#include "express/Response.h"
#include "express/State.h"
#include "express/dispatcher/RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Route::Route()
        : mountPoint("use", "/")
        , dispatcher(std::make_shared<express::dispatcher::RouterDispatcher>()) {
    }

    Route::Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher)
        : mountPoint(method, relativeMountPath)
        , dispatcher(dispatcher) {
    }

    void Route::dispatch(State& state) {
        if (!dispatch(state, "")) {
            state.response->sendStatus(501);
        }
    }

    bool Route::dispatch(State& state, const std::string& parentMountPath) {
        state.currentRoute = this;

        bool dispatched = dispatcher->dispatch(state, parentMountPath, mountPoint);

        return dispatched;
    }

    bool Route::breakDispatch(State& state) {
        bool breakRouting = false;

        if (this == state.lastRoute) {
            state.flags &= ~State::INH;
            if ((state.flags & State::NXT) != 0) {
                state.flags &= ~State::NXT;
                breakRouting = true;
            }
        }

        return breakRouting;
    }

} // namespace express
