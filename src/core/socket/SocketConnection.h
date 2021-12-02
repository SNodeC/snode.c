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

#ifndef CORE_SOCKET_SOCKETCONNECTION_H
#define CORE_SOCKET_SOCKETCONNECTION_H

namespace core::socket {
    class SocketContext;
    class SocketContextFactory;
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <memory>
#include <string>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    class SocketConnection {
        SocketConnection() = delete;
        SocketConnection(const core::socket::SocketConnection&) = delete;
        SocketConnection& operator=(const core::socket::SocketConnection&) = delete;

    private:
        using SocketContextFactory = core::socket::SocketContextFactory;

    protected:
        SocketConnection(const std::shared_ptr<SocketContextFactory>& socketContextFactory);

        virtual ~SocketConnection() = default;

    public:
        SocketContext* getSocketContext();

        virtual std::string getLocalAddressAsString() const = 0;
        virtual std::string getRemoteAddressAsString() const = 0;

        virtual void sendToPeer(const char* junk, std::size_t junkLen) = 0;
        virtual void sendToPeer(const std::string& data) = 0;

        virtual ssize_t readFromPeer(char* junk, std::size_t junkLen) = 0;

        virtual void close() = 0;

        virtual void shutdownRead() = 0;
        virtual void shutdownWrite() = 0;

        virtual void setTimeout(int timeout) = 0;

        virtual SocketContext* switchSocketContext(SocketContextFactory* socketContextFactory) = 0;

    protected:
        SocketContext* socketContext = nullptr;

        friend SocketContext;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETCONNECTION_H