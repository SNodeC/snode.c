/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DECLARE_ROUTE_REQUESTMETHOD(METHOD)                                                                                                \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda) const;             \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda) const;      \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda,                    \
                  Lambdas... lambdas) const;                                                                                               \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda,             \
                  Lambdas... lambdas) const;

namespace express {

    class Route {
    public:
        Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher);

        std::list<std::string> getRoute(const std::string& parentMountPath, bool strictRouting) const;

    private:
        Route();

        bool dispatch(Controller& controller, bool strictRouting, bool caseInsensitiveRouting, bool mergeParams);

        bool dispatch(
            Controller& controller, const std::string& parentMountPath, bool strictRouting, bool caseInsensitiveRouting, bool mergeParams);
        bool dispatchNext(
            Controller& controller, const std::string& parentMountPath, bool strictRouting, bool caseInsensitiveRouting, bool mergeParams);

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
