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

#include "HTTP10Response.h"

#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef MAX_CONTENT_CHUNK_LEN
#define MAX_CONTENT_CHUNK_LEN 16384
#endif

namespace web::http::decoder {

    HTTP10Response::HTTP10Response(const core::socket::stream::SocketContext *socketContext)
        : socketContext(socketContext) {
    }

    HTTP10Response::~HTTP10Response() {
    }

    std::size_t HTTP10Response::read() {
        std::size_t consumed = 0;
        std::size_t ret = 0;
        static char contentChunk[MAX_CONTENT_CHUNK_LEN];

        do {
            ret = socketContext->readFromPeer(contentChunk, MAX_CONTENT_CHUNK_LEN);
            contentLengthRead += ret;
            consumed += ret;

            content.insert(content.end(), contentChunk, contentChunk + ret);
        } while (ret > 0);

        return consumed;
    }

} // namespace web::http::decoder
