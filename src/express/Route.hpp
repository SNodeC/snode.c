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

#ifndef EXPRESS_ROUTE_HPP
#define EXPRESS_ROUTE_HPP

namespace express {

    class Next;
    class Request;
    class Response;
    class Route;

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional> // for function
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                           \
    template <typename... Lambdas>                                                                                                         \
    Route& Route::METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda,             \
                         Lambdas... lambdas) const {                                                                                       \
        return this->METHOD(lambda).METHOD(lambdas...);                                                                                    \
    }                                                                                                                                      \
    template <typename... Lambdas>                                                                                                         \
    Route& Route::METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda,      \
                         Lambdas... lambdas) const {                                                                                       \
        return this->METHOD(lambda).METHOD(lambdas...);                                                                                    \
    }

namespace express {

    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(use, "use")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(all, "all")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(get, "GET")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(put, "PUT")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(post, "POST")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(del, "DELETE")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(trace, "TRACE")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(patch, "PATCH")
    DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(head, "HEAD")

} // namespace express

#endif // EXPRESS_ROUTE_HPP
