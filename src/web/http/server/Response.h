/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

namespace web::http::server {
    class Request;
    class RequestContextBase;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>    // for size_t
#include <functional> // IWYU pragma: export
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class Response : public core::pipe::Sink {
    protected:
        explicit Response(RequestContextBase* requestContext);
        Response(const Response&) = default;

        ~Response() override;

    public:
        void send(const char* junk, std::size_t junkLen);
        void send(const std::string& junk);

        void stream(core::pipe::Source* source);

        void end();

        void sendResponse(const char* junk, std::size_t junkLen);
        void sendResponse(const std::string& junk);

        Response& status(int status);
        Response& append(const std::string& field, const std::string& value);
        Response& set(const std::string& field, const std::string& value, bool overwrite = true);
        Response& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
        Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});
        Response& type(const std::string& type);

        [[nodiscard]] bool upgrade(Request& req);

        void sendFile(const std::string& file, const std::function<void(int errnum)>& onError);

    protected:
        RequestContextBase* requestContext;

        int responseStatus = 200;

        std::map<std::string, std::string> headers;
        std::map<std::string, web::http::CookieOptions> cookies;

    private:
        void sendToPeerCompleted();
        core::pipe::Source* source = nullptr;
        ConnectionState connectionState = ConnectionState::Default;

        core::socket::stream::SocketContext* socketContextUpgrade = nullptr;

        bool sendHeaderInProgress = false;
        bool headersSent = false;

        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        void sendHeader();

        void receive(const char* junk, std::size_t junkLen) override;
        void eof() override;
        void error(int errnum) override;

        template <typename Request, typename Response>
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
