/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TLS_SOCKETSERVER_H
#define TLS_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketServer.h"
#include "socket/tls/SocketConnection.h"

namespace net::socket::tls {

    class SocketServer : public net::socket::SocketServer<tls::SocketConnection> {
    public:
        SocketServer(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError, const std::string& certChain,
                     const std::string& keyPEM, const std::string& password, const std::string& caFile = "");

    protected:
        using net::socket::SocketServer<SocketConnection>::SocketServer;

    private:
        ~SocketServer() override;

    public:
        void listen(in_port_t port, int backlog, const std::function<void(int err)>& onError);

    protected:
        using net::socket::SocketServer<SocketConnection>::listen;

    private:
        SSL_CTX* ctx;
        unsigned long sslErr = 0;
        static int passwordCallback(char* buf, int size, int rwflag, void* u);
    };

}; // namespace net::socket::tls

#endif // TLS_SOCKETSERVER_H
