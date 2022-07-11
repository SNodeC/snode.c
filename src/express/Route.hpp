/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef EXPRESS_ROUTE_HPP
#define EXPRESS_ROUTE_HPP

#include "express/Route.h" // IWYU pragma: export

namespace express {

    class Request;
    class Response;
    class Next;

} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DEFINE_ROUTE_TEMPLATE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                           \
    template <typename... Lambdas>                                                                                                         \
    Route& Route::METHOD(const std::function<void(Request & req, Response & res)>& lambda, Lambdas... lambdas) {                           \
        return this->METHOD(lambda).METHOD(lambdas...);                                                                                    \
    }                                                                                                                                      \
    template <typename... Lambdas>                                                                                                         \
    Route& Route::METHOD(const std::function<void(Request & req, Response & res, Next & state)>& lambda, Lambdas... lambdas) {             \
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
