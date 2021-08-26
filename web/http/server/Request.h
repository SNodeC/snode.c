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

#ifndef WEB_HTTP_SERVER_REQUEST_H
#define WEB_HTTP_SERVER_REQUEST_H

#include "utils/AttributeInjector.h"
#include "web/http/ConnectionState.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <map>     // for map
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class Request : public utils::MultibleAttributeInjector {
    protected:
        Request() = default;

        virtual ~Request() = default;

    public:
        const std::string& header(const std::string& key, int i = 0) const;
        const std::string& cookie(const std::string& key) const;
        const std::string& query(const std::string& key) const;
        std::size_t bodyLength() const;

        // Properties
        std::string method;
        std::string url;
        std::string httpVersion;
        int httpMajor = 0;
        int httpMinor = 0;
        char* body = nullptr;
        std::size_t contentLength = 0;

    protected:
        ConnectionState connectionState = ConnectionState::Default;

        virtual void reset();

        const std::map<std::string, std::string>* queries = nullptr;
        const std::map<std::string, std::string>* headers = nullptr;
        const std::map<std::string, std::string>* cookies = nullptr;

        std::string nullstr = "";

        template <typename Request, typename Response>
        friend class SocketContextT;
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
