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

#ifndef CORE_SOCKET_SOCKETCONNECTION_H
#define CORE_SOCKET_SOCKETCONNECTION_H

namespace utils {
    class Timeval;
} // namespace utils

namespace core::socket {
    class SocketAddress;
    class SocketContextFactory;
    namespace stream {
        class SocketContext;
    }
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    class SocketConnection {
    public:
        SocketConnection(const core::socket::SocketConnection&) = delete;

        SocketConnection& operator=(const core::socket::SocketConnection&) = delete;

    protected:
        SocketConnection() = default;
        virtual ~SocketConnection();

    public:
        virtual void sendToPeer(const char* junk, std::size_t junkLen) = 0;
        virtual void sendToPeer(const std::string& data) = 0;

        virtual std::size_t readFromPeer(char* junk, std::size_t junkLen) = 0;

        virtual const core::socket::SocketAddress& getLocalAddress() const = 0;
        virtual const core::socket::SocketAddress& getRemoteAddress() const = 0;

        virtual void close() = 0;

        virtual void shutdownRead() = 0;
        virtual void shutdownWrite(bool forceClose) = 0;

        virtual void setTimeout(const utils::Timeval& timeout) = 0;

    protected: // must be callable from subclasses
        void onConnected();
        void onDisconnected();

        void onWriteError(int errnum);
        void onReadError(int errnum);

        core::socket::stream::SocketContext* setSocketContext(core::socket::SocketContextFactory* socketContextFactory);

        core::socket::stream::SocketContext* switchSocketContext(core::socket::SocketContextFactory* socketContextFactory);

    protected:
        core::socket::stream::SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* newSocketContext = nullptr;

        friend class core::socket::stream::SocketContext;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETCONNECTION_H
