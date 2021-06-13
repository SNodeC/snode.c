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

#ifndef NET_SOCKET_STREAM_SOCKETCONNECTIONBASE_H
#define NET_SOCKET_STREAM_SOCKETCONNECTIONBASE_H

namespace net::socket::stream {
    class SocketContext;
    class SocketContextFactory;
} // namespace net::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    class SocketConnectionBase {
        SocketConnectionBase(const SocketConnectionBase&) = delete;
        SocketConnectionBase& operator=(const SocketConnectionBase&) = delete;

    protected:
        SocketConnectionBase() = default;

        virtual ~SocketConnectionBase() = default;

    public:
        virtual SocketContext* getSocketContext() = 0;

        virtual std::string getLocalAddressAsString() const = 0;
        virtual std::string getRemoteAddressAsString() const = 0;

        virtual void enqueue(const char* junk, std::size_t junkLen) = 0;
        virtual void enqueue(const std::string& data) = 0;

        virtual std::size_t doRead(char* junk, std::size_t junkLen) = 0;

        virtual void close() = 0;

        virtual void setTimeout(int timeout) = 0;

    protected:
        virtual void switchSocketProtocol(const SocketContextFactory& socketContextFactory) = 0;

        friend SocketContext;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCONNECTIONBASE_H
