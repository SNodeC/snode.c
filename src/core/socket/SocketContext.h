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

#ifndef CORE_SOCKET_SOCKETCONTEXT_H
#define CORE_SOCKET_SOCKETCONTEXT_H

// IWYU pragma: no_include "core/socket/SocketConnection.h"
#include "core/socket/Socket.h" // IWYU pragma: export

namespace utils {
    class Timeval;
}

namespace core::socket {
    class SocketConnection;     // IWYU pragma: keep
    class SocketContextFactory; // IWYU pragma: keep
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    class SocketContext {
    protected:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        virtual ~SocketContext() = default;

    public:
        void setTimeout(const utils::Timeval& timeout);
        Socket& getSocket();

        void sendToPeer(const char* junk, std::size_t junkLen);
        void sendToPeer(const std::string& data);
        std::size_t readFromPeer(char* junk, std::size_t junklen);

        void shutdownRead();
        void shutdownWrite(bool forceClose = false);
        void shutdown(bool forceClose = false);

        void close();

        SocketContext* switchSocketContext(core::socket::SocketContextFactory* socketContextFactory);

    private:
        virtual void onConnected();
        virtual void onDisconnected();

        virtual std::size_t onReceiveFromPeer() = 0;

        virtual void onWriteError(int errnum);
        virtual void onReadError(int errnum);

        core::socket::SocketConnection* socketConnection;

        friend class core::socket::SocketConnection;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETCONTEXT_H
