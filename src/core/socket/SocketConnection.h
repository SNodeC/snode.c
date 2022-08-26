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

#ifndef CORE_SOCKET_SOCKETCONNECTION_H
#define CORE_SOCKET_SOCKETCONNECTION_H

namespace utils {
    class Timeval;
}

namespace core::socket {
    class SocketContextFactory;
    class SocketContext;
    class Socket;
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
        core::socket::SocketContext* getSocketContext();

        virtual Socket& getSocket() = 0;

        virtual void close() = 0;

        virtual void shutdownRead() = 0;
        virtual void shutdownWrite(bool forceClose) = 0;

        virtual void setTimeout(const utils::Timeval& timeout) = 0;

    protected: // must be callable from subclasses
        void onConnected();
        void onDisconnected();

        std::size_t onReceiveFromPeer();

        void onWriteError(int errnum);
        void onReadError(int errnum);

        core::socket::SocketContext* setSocketContext(core::socket::SocketContextFactory* socketContextFactory);

    public: // will be called class SocketContext
        virtual void sendToPeer(const char* junk, std::size_t junkLen) = 0;
        virtual void sendToPeer(const std::string& data) = 0;

        virtual std::size_t readFromPeer(char* junk, std::size_t junkLen) = 0;

        core::socket::SocketContext* switchSocketContext(core::socket::SocketContextFactory* socketContextFactory);

    private:
        core::socket::SocketContext* socketContext = nullptr;
        core::socket::SocketContext* newSocketContext = nullptr;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETCONNECTION_H
