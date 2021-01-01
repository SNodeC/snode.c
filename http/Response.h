/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ConnectionState.h"
#include "CookieOptions.h"
#include "streams/WriteStream.h"

class FileReader;

namespace http {

    class ServerContextBase;

    class Response : public net::stream::WriteStream {
    protected:
        explicit Response(ServerContextBase* serverContext);

    public:
        void send(const char* buffer, std::size_t size);
        void send(const std::string& text);

        void end();

        Response& status(int status);
        Response& append(const std::string& field, const std::string& value);
        Response& set(const std::string& field, const std::string& value, bool overwrite = false);
        Response& set(const std::map<std::string, std::string>& headers, bool overwrite = false);
        Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
        Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});
        Response& type(const std::string& type);

    protected:
        ServerContextBase* serverContext;

        ConnectionState connectionState = ConnectionState::Default;

        bool sendHeaderInProgress = false;
        bool headersSent = false;

        int responseStatus = 200;

        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        void enqueue(const char* buf, std::size_t len);
        void enqueue(const std::string& str);
        void sendHeader();

        void pipe(net::stream::ReadStream& readStream, const char* junk, std::size_t junkLen) override;
        void pipeEOF([[maybe_unused]] net::stream::ReadStream& readStream) override;
        void pipeError(net::stream::ReadStream& readStream, int errnum) override;

        virtual void reset();

        std::map<std::string, std::string> headers;
        std::map<std::string, CookieOptions> cookies;

        template <typename Request, typename Response>
        friend class ServerContext;
    };

} // namespace http

#endif // RESPONSE_H

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
