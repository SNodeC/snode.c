/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

    ApplicationDispatcher::ApplicationDispatcher(
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda)
        : lambda(lambda) {
    }

    bool ApplicationDispatcher::dispatch(express::Controller& controller,
                                         const std::string& parentMountPath,
                                         const express::MountPoint& mountPoint) {
        bool requestMatched = false;

        if ((controller.getFlags() & Controller::NEXT) == 0) {
            const std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

            const std::string requestMethod = controller.getRequest()->method;
            const std::string requestUrl = controller.getRequest()->url;
            const std::string requestPath = controller.getRequest()->path;

            // clang-format off
            requestMatched =
                (
                    (
                        (
                            mountPoint.method == requestMethod
                        ) || (
                            mountPoint.method == "all"
                        )
                    ) && (
                        (
                            absoluteMountPath == requestUrl
                        ) || (
                            (
                                !controller.getStrictRouting()
                            ) && (
                                absoluteMountPath == requestPath
                            )
                        ) || (
                            checkForUrlMatch(absoluteMountPath, requestUrl)
                        )
                    )
                ) || (
                    (
                        mountPoint.method == "use"
                    ) && (
                        (
                            requestUrl.starts_with(absoluteMountPath)
                        ) || (
                            (
                                !controller.getStrictRouting()
                            ) && (
                                requestPath.starts_with(absoluteMountPath)
                            )
                        )
                    )
                );
            // clang-format on

            if (requestMatched) {
                if (hasResult(absoluteMountPath)) {
                    setParams(absoluteMountPath, *controller.getRequest());
                }

                lambda(controller.getRequest(), controller.getResponse());
            }
        }

        return requestMatched;
    }

} // namespace express::dispatcher
