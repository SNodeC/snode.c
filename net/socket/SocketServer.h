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

#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cerrno>
#include <cstdlib>
#include <easylogging++.h>
#include <functional>
#include <map>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AcceptEventReceiver.h"
#include "Logger.h"
#include "ReadEventReceiver.h"
#include "Socket.h"
#include "SocketListener.h"

namespace net::socket {

    template <typename SocketConnectionT>
    class SocketServer {
    public:
        using SocketConnection = SocketConnectionT;

        void* operator new(size_t size) {
            SocketServer<SocketConnection>::lastAllocAddress = malloc(size);

            return SocketServer<SocketConnection>::lastAllocAddress;
        }

        void operator delete(void* socketServer_v) {
            free(socketServer_v);
        }

        SocketServer(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                     const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
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

        void listen(const InetAddress& localAddress, int backlog, const std::function<void(int err)>& onError) {
            SocketListener<SocketConnection>* socketListener =
                new SocketListener<SocketConnection>(onConnect, onDisconnect, onRead, onReadError, onWriteError);

            socketListener->open([&socketListener, &localAddress, &backlog, &onError](int errnum) -> void {
                if (errnum > 0) {
                    onError(errnum);
                } else {
                    socketListener->reuseAddress([&socketListener, &localAddress, &backlog, &onError](int errnum) -> void {
                        if (errnum != 0) {
                            onError(errnum);
                        } else {
                            socketListener->bind(localAddress, [&socketListener, &backlog, &onError](int errnum) -> void {
                                if (errnum > 0) {
                                    onError(errnum);
                                } else {
                                    int ret = ::listen(socketListener->getFd(), backlog);

                                    if (ret < 0) {
                                        onError(errno);
                                    } else {
                                        socketListener->AcceptEventReceiver::enable();
                                        onError(0);
                                    }
                                }
                            });
                        }
                    });
                }
            });
        }

        void listen(in_port_t port, int backlog, const std::function<void(int err)>& onError) {
            listen(InetAddress(port), backlog, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, int backlog, const std::function<void(int err)>& onError) {
            listen(InetAddress(ipOrHostname, port), backlog, onError);
        }

    private:
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

    template <typename SocketConnection>
    void* SocketServer<SocketConnection>::lastAllocAddress = nullptr;

} // namespace net::socket

#endif // SOCKETSERVER_H
