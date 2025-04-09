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

#include "express/Router.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DEFINE_ROUTER_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                   \
    Route& Router::METHOD(const Router& router) const {                                                                                    \
        return rootRoute->METHOD(*router.rootRoute.get());                                                                                 \
    }                                                                                                                                      \
    Route& Router::METHOD(const std::string& relativeMountPath, const Router& router) const {                                              \
        return rootRoute->METHOD(relativeMountPath, *router.rootRoute.get());                                                              \
    }                                                                                                                                      \
    Route& Router::METHOD(const std::string& relativeMountPath,                                                                            \
                          const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda)     \
        const {                                                                                                                            \
        return rootRoute->METHOD(relativeMountPath, lambda);                                                                               \
    }                                                                                                                                      \
    Route& Router::METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda)     \
        const {                                                                                                                            \
        return rootRoute->METHOD(lambda);                                                                                                  \
    }                                                                                                                                      \
    Route& Router::METHOD(const std::string& relativeMountPath,                                                                            \
                          const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda) const {    \
        return rootRoute->METHOD(relativeMountPath, lambda);                                                                               \
    }                                                                                                                                      \
    Route& Router::METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda) const {    \
        return rootRoute->METHOD(lambda);                                                                                                  \
    }

namespace express {

    Router::Router()
        : rootRoute(std::make_shared<RootRoute>()) {
    }

    const Router& Router::setStrictRouting(bool strictRouting) const {
        rootRoute->setStrictRouting(strictRouting);

        return *this;
    }

    DEFINE_ROUTER_REQUESTMETHOD(use, "use")
    DEFINE_ROUTER_REQUESTMETHOD(all, "all")
    DEFINE_ROUTER_REQUESTMETHOD(get, "GET")
    DEFINE_ROUTER_REQUESTMETHOD(put, "PUT")
    DEFINE_ROUTER_REQUESTMETHOD(post, "POST")
    DEFINE_ROUTER_REQUESTMETHOD(del, "DELETE")
    DEFINE_ROUTER_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_ROUTER_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_ROUTER_REQUESTMETHOD(trace, "TRACE")
    DEFINE_ROUTER_REQUESTMETHOD(patch, "PATCH")
    DEFINE_ROUTER_REQUESTMETHOD(head, "HEAD")

} // namespace express
