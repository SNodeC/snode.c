/*
 * snode.c - a slim toolkit for network communication
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

#ifndef WEB_HTTP_SERVER_RESPONSE_H
#define WEB_HTTP_SERVER_RESPONSE_H

#include "core/pipe/Sink.h"
#include "web/http/ConnectionState.h"
#include "web/http/CookieOptions.h"

namespace core {
    namespace pipe {
        class Source;
    }
    namespace socket::stream {
        class SocketContext;
    }
} // namespace core

namespace web::http {
    namespace server {
        class Request;
        class SocketContext;
    } // namespace server
} // namespace web::http

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>    // for size_t
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
        explicit Response(Response&&) noexcept = default;

        Response& operator=(Response&) = delete;
        Response& operator=(Response&&) noexcept = default;

        ~Response() override;

        void stopResponse();

    private:
        virtual void init();

    public:
        Response& status(int status);
        Response& append(const std::string& field, const std::string& value);
        Response& set(const std::string& field, const std::string& value, bool overwrite = true);
        Response& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Response& type(const std::string& type);
        Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
        Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});

        void send(const char* junk, std::size_t junkLen);
        void send(const std::string& junk);
        void upgrade(const std::shared_ptr<Request>& request, const std::function<void(bool success)>& status);
        void sendFile(const std::string& file, const std::function<void(int errnum)>& callback);
        void end();

        Response& sendHeader();
        Response& sendFragment(const char* junk, std::size_t junkLen);
        Response& sendFragment(const std::string& junk);

    private:
        void sendCompleted();

        void onSourceConnect(core::pipe::Source* source) override;
        void onSourceData(const char* junk, std::size_t junkLen) override;
        void onSourceEof() override;
        void onSourceError(int errnum) override;

    public:
        const std::string& header(const std::string& field);

        SocketContext* getSocketContext() const;

        int statusCode = 200;

    protected:
        std::map<std::string, std::string> headers;
        std::map<std::string, web::http::CookieOptions> cookies;

    private:
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* socketContextUpgrade = nullptr;

        ConnectionState connectionState = ConnectionState::Default;

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
