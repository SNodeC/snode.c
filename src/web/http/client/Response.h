/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef WEB_HTTP_CLIENT_RESPONSE_H
#define WEB_HTTP_CLIENT_RESPONSE_H

#include "web/http/CookieOptions.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // IWYU pragma: export
#include <map>
#include <string>
#include <vector> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {
    class SocketContext;

    namespace client {
        class Request;
    } // namespace client
} // namespace web::http

namespace web::http::client {

    class Response {
    protected:
        explicit Response(web::http::SocketContext* clientContext);

        web::http::SocketContext* socketContext;

        void reset();

        // switch to protected later on
    public:
        const std::string& header(const std::string& key, int i = 0) const;
        const std::string& cookie(const std::string& key) const;

        std::string httpVersion;
        std::string statusCode;
        std::string reason;
        std::vector<uint8_t> body;

        void upgrade(Request& request);

        // need code to make it at least protected
        std::map<std::string, std::string> headers;
        std::map<std::string, CookieOptions> cookies;

        // CookieOptions are not queryable currently. Need code to access it.

        template <typename Request, typename Response>
        friend class SocketContext;

    private:
        std::string nullstr = "";
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_RESPONSE_H
