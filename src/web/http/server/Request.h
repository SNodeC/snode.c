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

#ifndef WEB_HTTP_SERVER_REQUEST_H
#define WEB_HTTP_SERVER_REQUEST_H

#include "web/http/ConnectionState.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/CiStringMap.h" // IWYU pragma: export

#include <cstdint> // IWYU pragma: export
#include <map>     // IWYU pragma: export
#include <string>
#include <vector> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class Request {
    public:
        Request() = default;

        explicit Request(Request&) = delete;
        explicit Request(Request&&) noexcept = default;

        Request& operator=(Request&) = delete;
        Request& operator=(Request&&) noexcept = delete;

        const std::string& get(const std::string& key, int i = 0) const;
        const std::string& cookie(const std::string& key) const;
        const std::string& query(const std::string& key) const;

    protected:
        ConnectionState connectionState = ConnectionState::Default;

    public:
        // Properties
        std::string method;
        std::string url;
        std::string httpVersion;
        int httpMajor = 0;
        int httpMinor = 0;

        CiStringMap<std::string> queries;
        CiStringMap<std::string> headers;
        CiStringMap<std::string> cookies;
        std::vector<char> body;

    private:
        std::string nullstr;

        friend class SocketContext;

        friend class RequestParser;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_REQUEST_H

/* https://de.wikipedia.org/wiki/Liste_der_HTTP-Headerfelder

Valid request header fields
===========================
X-                              https://tools.ietf.org/html/rfc6648
Accept
Accept-Charset
Accept-Encoding
Accept-Language
Authorization
Cache-Control
Connection
Cookie
Content-Length
Content-MD5
Content-Type
Date
Expect
Forwarded
From
Host
If-Match
If-Modified-Since
If-None-Match
If-Range
If-Unmodified-Since
Max-Forwards
Pragma
Proxy-Authorization
Range
Referer
TE
Transfer-Encoding
Upgrade
User-Agent
Via
Warning

Non standard request header fields
==================================
X-Requested-With
X-Do-Not-Track
DNT
X-Forwarded-For
X-Forwarded-Proto
*/
