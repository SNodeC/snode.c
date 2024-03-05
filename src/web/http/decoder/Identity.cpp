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

#include "web/http/decoder/Identity.h"

#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    Identity::Identity(const core::socket::stream::SocketContext* socketContext, std::size_t contentLength)
        : socketContext(socketContext)
        , contentLength(contentLength) {
        content.resize(contentLength);
    }

    Identity::~Identity() {
    }

    std::size_t Identity::read() {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        switch (state) {
            case -1:
                content.clear();
                completed = false;
                error = false;

                state = 0;

                [[fallthrough]];
            case 0:
                do {
                    ret = socketContext->readFromPeer(content.data() + contentLengthRead, contentLength - contentLengthRead);
                    contentLengthRead += ret;
                    consumed += ret;
                    completed = contentLength == contentLengthRead;
                } while (ret > 0 && !completed);

                if (completed) {
                    state = -1;
                }

                break;
        }
        return consumed;
    }

} // namespace web::http::decoder
