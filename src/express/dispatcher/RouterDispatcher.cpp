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

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    bool RouterDispatcher::dispatch(
        State& state, const std::string& parentMountPath, const MountPoint& mountPoint, Request& req, Response& res) {
        VLOG(0) << "###### Start Dispatch: " << this;
        bool dispatched = false;

        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        state.found = true;
        std::list<Route>::iterator it = routes.begin();

        if (state.resumeOnNext || state.resumeOnParent) {
            state.found = false;
            it = std::find_if(routes.begin(), routes.end(), [&state](Route& route) -> bool {
                return &route == state.lastRoute;
            });

            if (it == routes.end()) {
                it = routes.begin();
                state.found = false;
            } else {
                state.found = true;
                ++it;
            }

            if (!state.found) {
                for (; it != routes.end(); ++it) {
                    if (std::dynamic_pointer_cast<RouterDispatcher>(it->dispatcher) != nullptr) {
                        dispatched = it->dispatch(state, absoluteMountPath, req, res);
                        if (state.found) {
                            ++it;
                            break;
                        }
                    }
                }
            }

            if (!state.found) {
                it = routes.begin();
            }
        }

        if (!dispatched && !state.resumeOnParent && state.found) {
            state.resumeOnNext = false;
            state.resumeOnParent = false;

            // TODO: Fix regex-match
            VLOG(0) << "###### AbsoluteMountPath: this = " << this << ", " << absoluteMountPath;
            req.path.rfind(absoluteMountPath, 0);

            [[maybe_unused]] bool b = mountPoint.method == "use";
            b = req.method == mountPoint.method;
            b = mountPoint.method == "all";

            if ((req.path.rfind(absoluteMountPath, 0) == 0 &&
                 (mountPoint.method == "use" || req.method == mountPoint.method || mountPoint.method == "all"))) {
                for (; it != routes.end(); ++it) {
                    state.lastRoute = &*it;
                    state.request = &req;
                    state.response = &res;

                    dispatched = it->dispatch(state, absoluteMountPath, req, res);

                    if (dispatched) {
                        break;
                    }
                }
            }
        }

        VLOG(0) << "###### End Dispatch: " << this;

        return dispatched;
    }

} // namespace express::dispatcher
