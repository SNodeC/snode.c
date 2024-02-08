/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef EXPRESS_DISPATCHER_CONTROLLER_H
#define EXPRESS_DISPATCHER_CONTROLLER_H

namespace express {
    class Request;
    class Response;
    class RootRoute;
    class Route;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Controller {
    public:
        Controller(std::shared_ptr<Request> request, std::shared_ptr<Response> response);

        void setRootRoute(RootRoute* rootRoute);
        void setCurrentRoute(Route* currentRoute);

        std::shared_ptr<Request> getRequest() const;
        std::shared_ptr<Response> getResponse() const;

        int getFlags() const;

        void next(const std::string& how) const;
        bool nextRouter();
        bool dispatchNext(const std::string& parentMountPath);

        bool laxRouting();

        enum Flags { NONE = 0, NEXT = 1 << 0, NEXT_ROUTE = 1 << 1, NEXT_ROUTER = 1 << 2 };

    private:
        RootRoute* rootRoute = nullptr;

        mutable Route* lastRoute = nullptr;
        Route* currentRoute = nullptr;

        unsigned long lastTick = 0;

        std::shared_ptr<Request> request = nullptr;
        std::shared_ptr<Response> response = nullptr;

        mutable int flags = NONE;
    };

} // namespace express

#endif // EXPRESS_DISPATCHER_CONTROLLER_H
