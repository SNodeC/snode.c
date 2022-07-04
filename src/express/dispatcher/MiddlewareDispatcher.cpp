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

#include "MiddlewareDispatcher.h"

#include "express/MountPoint.h" // for MountPoint
#include "express/Request.h"
#include "express/State.h" // for State, State::INH
#include "express/dispatcher/regex_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    MiddlewareDispatcher::MiddlewareDispatcher(const std::function<void(express::Request&, express::Response&, express::Next&&)>& lambda)
        : lambda(lambda) {
    }

    bool MiddlewareDispatcher::dispatch(express::State& state, const std::string& parentMountPath, const MountPoint& mountPoint) {
        bool dispatched = false;

        if ((state.getFlags() & State::INH) == 0) {
            std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

            // TODO: Fix regex-match
            if ((state.getRequest()->path.rfind(absoluteMountPath, 0) == 0 && mountPoint.method == "use") ||
                ((absoluteMountPath == state.getRequest()->path || checkForUrlMatch(absoluteMountPath, state.getRequest()->url)) &&
                 (state.getRequest()->method == mountPoint.method || mountPoint.method == "all"))) {
                dispatched = true;

                if (hasResult(absoluteMountPath)) {
                    setParams(absoluteMountPath, *state.getRequest());
                }

                lambda(*state.getRequest(), *state.getResponse(), Next(state));
            }
        }

        return dispatched;
    }

} // namespace express::dispatcher
