
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

#ifndef SOCKETSERVERNEW_H
#define SOCKETSERVERNEW_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cerrno>
#include <functional>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "SocketListener.h"

namespace net::socket::stream {

    template <typename SocketListenerT>
    class SocketServer {
    public:
        using SocketListener = SocketListenerT;
        using SocketConnection = typename SocketListener::SocketConnection;

        void* operator new(size_t size) {
            SocketServer<SocketListener>::lastAllocAddress = malloc(size);

            return SocketServer<SocketListener>::lastAllocAddress;
        }

        void operator delete(void* socketServer_v) {
            free(socketServer_v);
        }

        SocketServer(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                     const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                     const std::function<void(SocketConnection* socketConnection)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                     const std::map<std::string, std::any>& options = {{}})
            : onConstruct(onConstruct)
            , onDestruct(onDestruct)
            , onConnect(onConnect)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError)
            , options(options)
            , isDynamic(this == SocketServer::lastAllocAddress) {
        }

        SocketServer() = delete;
        SocketServer(const SocketServer&) = delete;

        SocketServer& operator=(const SocketServer&) = delete;

        virtual ~SocketServer() = default;

    public:
        void listen(const typename SocketConnection::Socket::SocketAddress& localAddress,
                    int backlog,
                    const std::function<void(int err)>& onError) {
            SocketListener* socketListener =
                new SocketListener(onConstruct, onDestruct, onConnect, onDisconnect, onRead, onReadError, onWriteError, options);

            socketListener->listen(localAddress, backlog, onError);
        }

        void listen(unsigned short port, int backlog, const std::function<void(int err)>& onError) {
            listen(typename SocketConnection::Socket::SocketAddress(port), backlog, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, int backlog, const std::function<void(int err)>& onError) {
            listen(typename SocketConnection::Socket::SocketAddress(ipOrHostname, port), backlog, onError);
        }

    private:
        std::function<void(SocketConnection* socketConnection)> onConstruct;
        std::function<void(SocketConnection* socketConnection)> onDestruct;
        std::function<void(SocketConnection* socketConnection)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;

    protected:
        std::map<std::string, std::any> options;

        bool isDynamic;
        static void* lastAllocAddress;
    };

    template <typename SocketListener>
    void* SocketServer<SocketListener>::lastAllocAddress = nullptr;

} // namespace net::socket::stream

#endif // SOCKETSERVERNEW_H
