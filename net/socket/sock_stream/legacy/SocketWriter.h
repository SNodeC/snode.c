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

#ifndef NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETWRITER_H
#define NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <sys/socket.h>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/sock_stream/SocketWriter.h"

namespace net::socket::stream::legacy {

    template <typename SocketT>
    class SocketWriter : public stream::SocketWriter<SocketT> {
        using stream::SocketWriter<SocketT>::SocketWriter;

    private:
        ssize_t write(const char* junk, size_t junkLen) override {
            return ::send(this->getFd(), junk, junkLen, MSG_NOSIGNAL);
        }

        int getError() override {
            return errno;
        }
    };

}; // namespace net::socket::stream::legacy

#endif // NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETWRITER_H
