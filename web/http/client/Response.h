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

#ifndef WEB_HTTP_CLIENT_RESPONSE_H
#define WEB_HTTP_CLIENT_RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <map>     // for map
#include <string>  // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {
    class CookieOptions;
}

namespace web::http::client {

    class Response {
    protected:
        Response() = default;

        // switch to protected later on
    public:
        void reset();

        std::string httpVersion;
        std::string statusCode;
        std::string reason;
        char* body = nullptr;
        std::size_t contentLength = 0;
        const std::map<std::string, std::string>* headers = nullptr;
        const std::map<std::string, CookieOptions>* cookies = nullptr;

        template <typename Request, typename Response>
        friend class SocketContext;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_RESPONSE_H
