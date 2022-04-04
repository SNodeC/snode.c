/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "web/http/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    void web::http::SocketContext::sendToPeer(const char* junk, std::size_t junkLen, bool isContent) {
        Super::sendToPeer(junk, junkLen);

        if (isContent) {
            contentSent += junkLen;
            if (contentSent == contentLength) {
                sendToPeerCompleted();
            } else if (contentSent > contentLength) {
                close();
            }
        }
    }

    void SocketContext::receive(const char* junk, std::size_t junkLen) {
        sendToPeer(junk, junkLen, true);
    }

} // namespace web::http
