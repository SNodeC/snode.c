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

#ifndef WEB_HTTP_SERVER_RESPONSE_H
#define WEB_HTTP_SERVER_RESPONSE_H

#include "core/pipe/Sink.h"
#include "web/http/ConnectionState.h"
#include "web/http/CookieOptions.h"
#include "web/http/TransferEncoding.h"

namespace core::socket::stream {
    class SocketContext;
} // namespace core::socket::stream

namespace web::http::server {
    class Request;
    class SocketContext;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/CiStringMap.h" // IWYU pragma: export

#include <cstddef>
#include <functional> // IWYU pragma: export
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class Response : public core::pipe::Sink {
    public:
        explicit Response(SocketContext* socketContext);

        explicit Response(Response&) = delete;
        explicit Response(Response&&) noexcept = delete;

        Response& operator=(Response&) = delete;
        Response& operator=(Response&&) noexcept = delete;

        ~Response() override;

        void stopResponse();

    private:
        virtual void init();

    public:
        Response& status(int statusCode);
        Response& append(const std::string& field, const std::string& value);
        Response& set(const std::string& field, const std::string& value, bool overwrite = true);
        Response& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Response& type(const std::string& type);
        Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
        Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});
        Response& setTrailer(const std::string& field, const std::string& value, bool overwrite = true);

        void send(const char* chunk, std::size_t chunkLen);
        void send(const std::string& chunk);
        void sendStatus(int statusCode);
        void upgrade(const std::shared_ptr<Request>& request, const std::function<void(const std::string&)>& status, int val);
        void sendFile(const std::string& file, const std::function<void(int)>& callback);
        void end();

        Response& sendHeader();
        Response& sendFragment(const char* chunk, std::size_t chunkLen);
        Response& sendFragment(const std::string& chunk);

    private:
        void sendCompleted();

        void onSourceConnect(core::pipe::Source* source) override;
        void onSourceData(const char* chunk, std::size_t chunkLen) override;
        void onSourceEof() override;
        void onSourceError(int errnum) override;

    public:
        const std::string& header(const std::string& field);

        SocketContext* getSocketContext() const;

        int statusCode = 200;
        int httpMajor = 1;
        int httpMinor = 1;

    protected:
        web::http::CiStringMap<std::string> headers;
        web::http::CiStringMap<web::http::CookieOptions> cookies;
        web::http::CiStringMap<std::string> trailer;

    private:
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* socketContextUpgrade = nullptr;

        ConnectionState connectionState = ConnectionState::Default;
        TransferEncoding transferEncoding = TransferEncoding::HTTP10;

        friend class SocketContext;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_RESPONSE_H

/* https://de.wikipedia.org/wiki/Liste_der_HTTP-Headerfelder

Valid response header fields
============================
X-                              https://tools.ietf.org/html/rfc6648
Accept-Ranges
Age
Allow
Cache-Control
Connection
Content-Encoding
Content-Language
Content-Length
Content-Location
Content-MD5
Content-Disposition
Content-Disposition:
Content-Range
Content-Security-Policy
Content-Type
Date
ETag
Expires
Last-Modified
Link
Location
P3P
Pragma
Proxy-Authenticate
Refresh
Retry-After
Server
Set-Cookie
Trailer
Transfer-Encoding
Vary
Via
Warning
WWW-Authenticate

Non standard response header fields
===================================
X-Frame-Options
X-XSS-Protection
X-Content-Type-Options
X-Powered-By
X-UA-Compatible
X-Robots-Tag
Referrer-Policy
*/
