/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#ifndef EXPRESS_ROUTER_H
#define EXPRESS_ROUTER_H

#include "express/Next.h"      // IWYU pragma: export
#include "express/Request.h"   // IWYU pragma: export
#include "express/Response.h"  // IWYU pragma: export
#include "express/RootRoute.h" // IWYU pragma: export

namespace express {
    class Router;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional> // IWYU pragma: export
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MIDDLEWARE(req, res, next)                                                                                                         \
    ([[maybe_unused]] const std::shared_ptr<express::Request>& req,                                                                        \
     [[maybe_unused]] const std::shared_ptr<express::Response>& res,                                                                       \
     [[maybe_unused]] express::Next&(next))

#define APPLICATION(req, res)                                                                                                              \
    ([[maybe_unused]] const std::shared_ptr<express::Request>& req, [[maybe_unused]] const std::shared_ptr<express::Response>& res)

#define DECLARE_ROUTER_REQUESTMETHOD(METHOD)                                                                                               \
    Route& METHOD(const Router& router) const;                                                                                             \
    Route& METHOD(const std::string& relativeMountPath, const Router& router) const;                                                       \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda) const;             \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda) const;             \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda) const;      \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda) const;      \
                                                                                                                                           \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda,                    \
                  Lambdas... lambdas) const;                                                                                               \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda,                    \
                  Lambdas... lambdas) const;                                                                                               \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda,             \
                  Lambdas... lambdas) const;                                                                                               \
    template <typename... Lambdas>                                                                                                         \
    Route& METHOD(const std::string& relativeMountPath,                                                                                    \
                  const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda,             \
                  Lambdas... lambdas) const;

namespace express {

    class Router {
    public:
        Router();
        Router(const Router&) = default;

        void strictRouting(bool strictRouting = true) const;

        DECLARE_ROUTER_REQUESTMETHOD(use)
        DECLARE_ROUTER_REQUESTMETHOD(all)
        DECLARE_ROUTER_REQUESTMETHOD(get)
        DECLARE_ROUTER_REQUESTMETHOD(put)
        DECLARE_ROUTER_REQUESTMETHOD(post)
        DECLARE_ROUTER_REQUESTMETHOD(del)
        DECLARE_ROUTER_REQUESTMETHOD(connect)
        DECLARE_ROUTER_REQUESTMETHOD(options)
        DECLARE_ROUTER_REQUESTMETHOD(trace)
        DECLARE_ROUTER_REQUESTMETHOD(patch)
        DECLARE_ROUTER_REQUESTMETHOD(head)

    protected:
        std::shared_ptr<RootRoute> rootRoute = nullptr;

        friend class Route;
        friend class RootRoute;
    };

} // namespace express

#include "express/Router.hpp" // IWYU pragma: export

#endif // EXPRESS_ROUTER_H
