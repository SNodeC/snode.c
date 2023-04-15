/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "express/dispatcher/ApplicationDispatcher.h"

#include "express/Controller.h"
#include "express/MountPoint.h"
#include "express/Request.h"
#include "express/dispatcher/regex_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    ApplicationDispatcher::ApplicationDispatcher(const std::function<void(express::Request&, express::Response&)>& lambda)
        : lambda(lambda) {
    }

    bool ApplicationDispatcher::dispatch(express::Controller& controller,
                                         const std::string& parentMountPath,
                                         const express::MountPoint& mountPoint) {
        bool dispatched = false;

        if ((controller.getFlags() & Controller::NEXT) == 0) {
            std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);
            if (((controller.getRequest()->path.rfind(absoluteMountPath, 0) == 0 ||
                  controller.getRequest()->url.rfind(absoluteMountPath, 0) == 0) &&
                 mountPoint.method == "use") ||
                ((absoluteMountPath == controller.getRequest()->url || absoluteMountPath == controller.getRequest()->path ||
                  checkForUrlMatch(absoluteMountPath, controller.getRequest()->url)) &&
                 (controller.getRequest()->method == mountPoint.method || mountPoint.method == "all"))) {
                dispatched = true;

                if (hasResult(absoluteMountPath)) {
                    setParams(absoluteMountPath, *controller.getRequest());
                }

                lambda(*controller.getRequest(), *controller.getResponse());
            }
        }

        return dispatched;
    }

} // namespace express::dispatcher
