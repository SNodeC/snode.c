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

#ifndef SOCKETLISTENER_H
#define SOCKETLISTENER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <easylogging++.h>
#include <functional>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AcceptEventReceiver.h"
#include "ReadEventReceiver.h"
#include "Socket.h"

namespace net::socket::tcp {

    template <typename SocketConnectionT>
    class SocketListener
        : public AcceptEventReceiver
        , public Socket {
    public:
        using SocketConnection = SocketConnectionT;

        void* operator new(size_t size) {
            SocketListener<SocketConnection>::lastAllocAddress = malloc(size);

            return SocketListener<SocketConnection>::lastAllocAddress;
        }

        void operator delete(void* socketListener_v) {
            free(socketListener_v);
        }

        SocketListener(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                       const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                       const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                       const std::map<std::string, std::any>& options)
            : AcceptEventReceiver()
            , Socket()
            , onConnect(onConnect)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError)
            , options(options)
            , isDynamic(this == SocketListener::lastAllocAddress) {
        }

        SocketListener() = delete;
        SocketListener(const SocketListener&) = delete;

        SocketListener& operator=(const SocketListener&) = delete;

        virtual ~SocketListener() = default;

        virtual void listen(const InetAddress& localAddress, int backlog, const std::function<void(int err)>& onError) {
            open([this, &localAddress, &backlog, &onError](int errnum) -> void {
                if (errnum > 0) {
                    onError(errnum);
                } else {
                    reuseAddress([this, &localAddress, &backlog, &onError](int errnum) -> void {
                        if (errnum != 0) {
                            onError(errnum);
                        } else {
                            bind(localAddress, [this, &backlog, &onError](int errnum) -> void {
                                if (errnum > 0) {
                                    onError(errnum);
                                } else {
                                    int ret = ::listen(getFd(), backlog);

                                    if (ret < 0) {
                                        onError(errno);
                                    } else {
                                        AcceptEventReceiver::enable();
                                        onError(0);
                                    }
                                }
                            });
                        }
                    });
                }
            });
        }

        void reuseAddress(const std::function<void(int errnum)>& onError) {
            int sockopt = 1;

            if (setsockopt(getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                onError(0);
            }
        }

        void acceptEvent() override {
            errno = 0;

            struct sockaddr_in remoteAddress {};
            socklen_t addrlen = sizeof(remoteAddress);

            int scFd = -1;

            scFd = ::accept4(getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &addrlen, SOCK_NONBLOCK);

            if (scFd >= 0) {
                struct sockaddr_in localAddress {};
                socklen_t addressLength = sizeof(localAddress);

                if (getsockname(scFd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                    SocketConnection* socketConnection = new SocketConnection(onRead, onReadError, onWriteError, onDisconnect);

                    socketConnection->open(scFd);

                    socketConnection->setRemoteAddress(InetAddress(remoteAddress));
                    socketConnection->setLocalAddress(InetAddress(localAddress));

                    socketConnection->ReadEventReceiver::enable();

                    onConnect(socketConnection);
                } else {
                    PLOG(ERROR) << "getsockname";
                    shutdown(scFd, SHUT_RDWR);
                    ::close(scFd);
                }
            } else if (errno != EINTR) {
                PLOG(ERROR) << "accept";
            }
        }

        void end() {
            AcceptEventReceiver::disable();
        }

    private:
        void unobserved() override {
            if (isDynamic) {
                delete this;
            }
        }

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
    void* SocketListener<SocketConnection>::lastAllocAddress = nullptr;

} // namespace net::socket::tcp

#endif // SOCKETLISTENER_H
