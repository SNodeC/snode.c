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

#ifndef EXPRESS_ROOTROUTE_H
#define EXPRESS_ROOTROUTE_H

#include "express/Route.h" // IWYU pragma: export

namespace express {
    class Request;
    class Response;
    class Next;
    class Controller;
    class RootRoute;

    namespace dispatcher {
        class RouterDispatcher;
    } // namespace dispatcher
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DECLARE_ROOTROUTE_REQUESTMETHOD(METHOD)                                                                                            \
    Route& METHOD(const RootRoute& rootRoute) const;                                                                                       \
    Route& METHOD(const std::string& relativeMountPath, const RootRoute& rootRoute) const;                                                 \
    Route& METHOD(const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res)>& lambda) const;                   \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res)>& lambda) const;                   \
    Route& METHOD(const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res, Next & next)>& lambda) const;      \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res, Next & next)>& lambda) const;

namespace express {

    class RootRoute : public Route {
    public:
        RootRoute() = default;

    protected:
        void dispatch(std::shared_ptr<Request>& req, std::shared_ptr<Response>& res);

        void dispatch(Controller&& controller);
        void dispatch(Controller& controller);

        std::shared_ptr<dispatcher::RouterDispatcher> getDispatcher() const;
        std::list<Route>& routes() const;

    public:
        DECLARE_ROOTROUTE_REQUESTMETHOD(use)
        DECLARE_ROOTROUTE_REQUESTMETHOD(all)
        DECLARE_ROOTROUTE_REQUESTMETHOD(get)
        DECLARE_ROOTROUTE_REQUESTMETHOD(put)
        DECLARE_ROOTROUTE_REQUESTMETHOD(post)
        DECLARE_ROOTROUTE_REQUESTMETHOD(del)
        DECLARE_ROOTROUTE_REQUESTMETHOD(connect)
        DECLARE_ROOTROUTE_REQUESTMETHOD(options)
        DECLARE_ROOTROUTE_REQUESTMETHOD(trace)
        DECLARE_ROOTROUTE_REQUESTMETHOD(patch)
        DECLARE_ROOTROUTE_REQUESTMETHOD(head)

        bool strict = true;

        friend class Route;
        friend class Controller;

        template <typename Server>
        friend class WebAppT;
    };

} // namespace express

#endif // EXPRESS_ROOTROUTE_H
