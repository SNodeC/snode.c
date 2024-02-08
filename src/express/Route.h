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

#ifndef EXPRESS_ROUTE_H
#define EXPRESS_ROUTE_H

#include "express/MountPoint.h" // IWYU pragma: export

namespace express {
    class Controller;
    class Dispatcher;
    class Request;
    class Response;
    class Next;
    class Route;

    namespace dispatcher {
        class RouterDispatcher;
    } // namespace dispatcher
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DECLARE_ROUTE_REQUESTMETHOD(METHOD)                                                                                                \
    Route& METHOD(const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res)>& lambda) const;                   \
    Route& METHOD(const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res, Next & next)>& lambda) const;      \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res)>& lambda, Lambdas... lambdas)      \
        const;                                                                                                                             \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res, Next & next)>& lambda,             \
                  Lambdas... lambdas) const;

namespace express {

    class Route {
    public:
        Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher);

    protected:
        Route();

        bool dispatch(Controller& controller);
        bool dispatch(Controller& controller, const std::string& parentMountPath);
        bool dispatchNext(Controller& controller, const std::string& parentMountPath);

    private:
        MountPoint mountPoint;
        std::shared_ptr<Dispatcher> dispatcher;

    public:
        DECLARE_ROUTE_REQUESTMETHOD(use)
        DECLARE_ROUTE_REQUESTMETHOD(all)
        DECLARE_ROUTE_REQUESTMETHOD(get)
        DECLARE_ROUTE_REQUESTMETHOD(put)
        DECLARE_ROUTE_REQUESTMETHOD(post)
        DECLARE_ROUTE_REQUESTMETHOD(del)
        DECLARE_ROUTE_REQUESTMETHOD(connect)
        DECLARE_ROUTE_REQUESTMETHOD(options)
        DECLARE_ROUTE_REQUESTMETHOD(trace)
        DECLARE_ROUTE_REQUESTMETHOD(patch)
        DECLARE_ROUTE_REQUESTMETHOD(head)

        friend class dispatcher::RouterDispatcher;
        friend class Dispatcher;
        friend class RootRoute;
        friend class Controller;
    };

} // namespace express

#include "express/Route.hpp" // IWYU pragma: export

#endif // EXPRESS_ROUTE_H
