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

#ifndef TLS_SOCKETLISTENER_H
#define TLS_SOCKETLISTENER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnection.h"
#include "socket/tcp/SocketListener.h"

namespace net::socket::tcp::tls {

    class SocketListener : public net::socket::tcp::SocketListener<net::socket::tcp::tls::SocketConnection> {
    public:
        using SocketConnection = net::socket::tcp::tls::SocketConnection;

        SocketListener(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                       const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                       const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                       const std::map<std::string, std::any>& options);

        ~SocketListener() override;

        void listen(const InetAddress& localAddress, int backlog, const std::function<void(int err)>& onError) override;

    private:
        SSL_CTX* ctx = nullptr;
        unsigned long sslErr = 0;
    };

} // namespace net::socket::tcp::tls

#endif // TLS_SOCKETLISTENER_H
