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

#ifndef WEB_HTTP_CLIENT_RESPONSE_H
#define WEB_HTTP_CLIENT_RESPONSE_H

#include "web/http/CookieOptions.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {
    class SocketContext;
}

namespace web::http::client {

    class Response {
    public:
        explicit Response(core::socket::stream::SocketContext* clientContext);

        explicit Response(Response&) = delete;
        explicit Response(Response&&) noexcept = default;

        Response& operator=(Response&) = delete;
        Response& operator=(Response&&) noexcept = default;

    protected:
        core::socket::stream::SocketContext* socketContext;

        void reset();

    public:
        const std::string& header(const std::string& key, int i = 0) const;
        const std::string& cookie(const std::string& key) const;

        // Properties
        std::string httpVersion;
        std::string statusCode;
        std::string reason;
        int httpMajor = 0;
        int httpMinor = 0;
        std::vector<uint8_t> body;

        std::map<std::string, std::string> headers;
        std::map<std::string, CookieOptions> cookies;

    private:
        std::string nullstr;

        template <typename Request, typename Response>
        friend class SocketContext;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_RESPONSE_H
