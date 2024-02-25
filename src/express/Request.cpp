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

#include "express/Request.h"

#include "web/http/server/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Request::Request(const std::shared_ptr<web::http::server::Request>& request) noexcept
        : requestBase(request)
        , method(request->method)
        , url(request->url)
        , httpVersion(request->httpVersion)
        , httpMajor(request->httpMajor)
        , httpMinor(request->httpMinor)
        , queries(std::move(request->queries))
        , headers(request->headers) // Do not move headers as they are possibly still needed in the source request
        , cookies(std::move(request->cookies))
        , body(std::move(request->body)) {
        extend();
    }

    const std::string& Request::param(const std::string& id, const std::string& fallBack) {
        return params.contains(id) ? params[id] : fallBack;
    }

    Request& Request::extend() {
        originalUrl = url;
        url = httputils::url_decode(httputils::str_split_last(originalUrl, '?').first);
        path = httputils::str_split_last(url, '/').first;
        if (path.empty()) {
            path = std::string("/");
        }

        return *this;
    }

    const std::string& Request::get(const std::string& key, int i) const {
        return requestBase->get(key, i);
    }

    const std::string& Request::cookie(const std::string& key) const {
        return requestBase->cookie(key);
    }

    const std::string& Request::query(const std::string& key) const {
        return requestBase->query(key);
    }

} // namespace express
