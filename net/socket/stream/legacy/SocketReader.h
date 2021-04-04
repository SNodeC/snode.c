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

#ifndef NET_SOCKET_STREAM_LEGACY_SOCKETREADER_H
#define NET_SOCKET_STREAM_LEGACY_SOCKETREADER_H

#include "net/socket/stream/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for std::size_t
#include <sys/socket.h>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream::legacy {

    template <typename SocketT>
    class SocketReader : public stream::SocketReader<SocketT> {
        using stream::SocketReader<SocketT>::SocketReader;

    private:
        ssize_t read(char* junk, std::size_t junkLen) override {
            return ::recv(this->getFd(), junk, junkLen, 0);
        }

        int getError() override {
            return errno;
        }
    };

} // namespace net::socket::stream::legacy

#endif // NET_SOCKET_STREAM_LEGACY_SOCKETREADER_H
