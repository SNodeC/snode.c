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

#ifndef EXPRESS_DISPATCHER_APPLICATIONDISPATCHER_H
#define EXPRESS_DISPATCHER_APPLICATIONDISPATCHER_H

#include "express/Dispatcher.h"

namespace express {
    class Request;
    class Response;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    class ApplicationDispatcher : public express::Dispatcher {
    public:
        explicit ApplicationDispatcher(
            const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda);

    private:
        bool dispatch(express::Controller& controller, const std::string& parentMountPath, const express::MountPoint& mountPoint) override;

        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)> lambda;
    };

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_APPLICATIONDISPATCHER_H
