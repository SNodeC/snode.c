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

#include "express/dispatcher/RouterDispatcher.h"

#include "express/Request.h" // for Request
#include "express/Route.h"
#include "express/dispatcher/regex_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "express/State.h" // for State, State::INH, State::NXT

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    bool RouterDispatcher::dispatch(express::State& state, const std::string& parentMountPath, const express::MountPoint& mountPoint) {
        bool dispatched = false;

        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        // TODO: Fix regex-match
        if ((state.getRequest()->path.rfind(absoluteMountPath, 0) == 0 &&
             (mountPoint.method == "use" || state.getRequest()->method == mountPoint.method || mountPoint.method == "all"))) {
            for (Route& route : routes) {
                dispatched = route.dispatch(state, absoluteMountPath);

                if (dispatched || state.next(route)) {
                    break;
                }
            }
        }

        return dispatched;
    }

} // namespace express::dispatcher
