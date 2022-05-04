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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    void RouterDispatcher::dispatch(Request& req, Response& res) const {
        dispatch(nullptr, "/", MountPoint("use", "/"), req, res);
    }

    void RouterDispatcher::dispatch(const RouterDispatcher* parentRouter,
                                    const std::string& parentMountPath,
                                    const MountPoint& mountPoint,
                                    Request& req,
                                    Response& res) const {
        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        // TODO: Fix regex-match
        if ((req.path.rfind(absoluteMountPath, 0) == 0 &&
             (mountPoint.method == "use" || req.method == mountPoint.method || mountPoint.method == "all"))) {
            for (const Route& route : routes) {
                route.dispatch(this, absoluteMountPath, req, res);

                if (!state.proceed) {
                    break;
                }
            }
        }

        returnTo(parentRouter);
    }

    void RouterDispatcher::terminate() const {
        state.proceed = false;
    }

    State& RouterDispatcher::getState() const {
        return state;
    }

    void RouterDispatcher::returnTo(const RouterDispatcher* parentRouter) const {
        if (parentRouter) {
            parentRouter->state.proceed = state.proceed | state.parentProceed;
        }

        state.proceed = true;
        state.parentProceed = false;
    }

} // namespace express::dispatcher
