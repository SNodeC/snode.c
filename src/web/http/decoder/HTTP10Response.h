/*
 * snode.c - a slim toolkit for network communication
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

#ifndef WEB_HTTP_DECODER_HTTP10_H
#define WEB_HTTP_DECODER_HTTP10_H

#include "web/http/ContentDecoder.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    class HTTP10Response : public web::http::ContentDecoder {
    public:
        explicit HTTP10Response(const core::socket::stream::SocketContext* socketContext);
        HTTP10Response(const HTTP10Response&) = delete;
        HTTP10Response(HTTP10Response&&) noexcept = default;

        HTTP10Response& operator=(const HTTP10Response&) = delete;
        HTTP10Response& operator=(HTTP10Response&&) noexcept = default;

        ~HTTP10Response() override;

    private:
        std::size_t read() override;

        const core::socket::stream::SocketContext* socketContext;

        std::size_t contentLengthRead = 0;
    };

} // namespace web::http::decoder

#endif // WEB_HTTP_DECODER_HTTP10_H
