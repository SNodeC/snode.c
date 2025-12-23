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

#include "express/Request.h"

#include "web/http/server/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <tuple>
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
        originalPath = httputils::url_decode(httputils::str_split_last(originalUrl, '?').first);

        if (originalPath.length() <= 2 || !originalPath.ends_with("/")) {
            std::tie(path, file) = httputils::str_split_last(originalPath, '/');

            if (path.empty()) {
                path = "/";
            }
        } else if (originalPath.ends_with("/")) {
            path = originalPath;
            path.pop_back();
        }

        return *this;
    }

    const std::string& Request::get(const std::string& key, int i) const {
        return requestBase->get(key, i);
    }

    const std::string& Request::cookie(const std::string& key) const {
        const std::map<std::string, std::string>::const_iterator it = cookies.find(key);

        if (it != cookies.end()) {
            return it->second;
        }

        return nullstr;
    }

    const std::string& Request::query(const std::string& key) const {
        const std::map<std::string, std::string>::const_iterator it = queries.find(key);

        if (it != queries.end()) {
            return it->second;
        }

        return nullstr;
    }

} // namespace express
