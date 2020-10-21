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

#ifndef NET_SOCKET_SOCK_STREAM_SOCKETCONNECTOR_H
#define NET_SOCKET_SOCK_STREAM_SOCKETCONNECTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <functional>
#include <map>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ConnectEventReceiver.h"
#include "ReadEventReceiver.h"
#include "socket/Socket.h"

namespace net::socket::stream {

    template <typename SocketConnectionT>
    class SocketConnector
        : public ConnectEventReceiver
        , public ReadEventReceiver
        , public SocketConnectionT::Socket {
    public:
        using SocketConnection = SocketConnectionT;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

    protected:
        void* operator new(size_t size) {
            SocketConnector<SocketConnection>::lastAllocAddress = malloc(size);

            return SocketConnector<SocketConnection>::lastAllocAddress;
        }

        void operator delete(void* socketListener_v) {
            free(socketListener_v);
        }

        SocketConnector(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                        const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                        const std::function<void(SocketConnection* socketConnection)>& onConnect,
                        const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                        const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                        const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                        const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                        const std::map<std::string, std::any>& options)
            : ConnectEventReceiver()
            , ReadEventReceiver()
            , SocketConnection::Socket()
            , onConstruct(onConstruct)
            , onDestruct(onDestruct)
            , onConnect(onConnect)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError)
            , options(options)
            , isDynamic(this == SocketConnector::lastAllocAddress) {
        }

        SocketConnector() = delete;
        SocketConnector(const SocketConnector&) = delete;

        SocketConnector& operator=(const SocketConnector&) = delete;

        virtual ~SocketConnector() = default;

        virtual void connect(const SocketAddress& remoteAddress,
                             const std::function<void(int err)>& onError,
                             const SocketAddress& bindAddress = SocketAddress()) {
            this->onError = onError;

            errno = 0;

            Socket::open(
                [this, &bindAddress, &remoteAddress, &onError](int errnum) -> void {
                    if (errnum > 0) {
                        onError(errnum);
                        destruct();
                    } else {
                        Socket::bind(bindAddress, [this, &remoteAddress, &onError](int errnum) -> void {
                            if (errnum > 0) {
                                onError(errnum);
                                destruct();
                            } else {
                                int ret = ::connect(Socket::getFd(), &remoteAddress.getSockAddr(), remoteAddress.getSockAddrLen());

                                if (ret == 0 || errno == EINPROGRESS) {
                                    ConnectEventReceiver::enable();
                                } else {
                                    onError(errno);
                                    destruct();
                                }
                            }
                        });
                    }
                },
                SOCK_NONBLOCK);
        }

    protected:
        std::function<void(int err)> onError;

    private:
        void connectEvent() override {
            errno = 0;
            int cErrno = 0;

            socklen_t cErrnoLen = sizeof(cErrno);

            int err = getsockopt(Socket::getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

            if (err == 0) {
                if (cErrno != EINPROGRESS) {
                    ConnectEventReceiver::disable();

                    if (cErrno == 0) {
                        typename Socket::SocketAddress::SockAddr localAddress{};
                        socklen_t localAddressLength = sizeof(localAddress);

                        typename Socket::SocketAddress::SockAddr remoteAddress{};
                        socklen_t remoteAddressLength = sizeof(remoteAddress);

                        if (getsockname(Socket::getFd(), reinterpret_cast<sockaddr*>(&localAddress), &localAddressLength) == 0 &&
                            getpeername(Socket::getFd(), reinterpret_cast<sockaddr*>(&remoteAddress), &remoteAddressLength) == 0) {
                            ReadEventReceiver::enable();

                            socketConnection =
                                new SocketConnection(onConstruct, onDestruct, onRead, onReadError, onWriteError, onDisconnect);

                            socketConnection->open(Socket::getFd(), Socket::FLAGS::dontClose);

                            socketConnection->setRemoteAddress(typename Socket::SocketAddress(remoteAddress));
                            socketConnection->setLocalAddress(typename Socket::SocketAddress(localAddress));

                            socketConnection->ReadEventReceiver::enable();

                            onError(0);
                            onConnect(socketConnection);
                        } else {
                            onError(errno);
                        }
                    } else {
                        errno = cErrno;
                        onError(errno);
                    }
                }
            } else {
                ConnectEventReceiver::disable();
                onError(errno);
            }
        }

        void readEvent() override {
            ReadEventReceiver::disable();
        }

        void unobserved() override {
            destruct();
        }

        void destruct() {
            if (isDynamic) {
                delete this;
            }
        }

        std::function<void(SocketConnection* socketConnection)> onConstruct;
        std::function<void(SocketConnection* socketConnection)> onDestruct;
        std::function<void(SocketConnection* socketConnection)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;

        SocketConnection* socketConnection = nullptr;

        std::map<std::string, std::any> options;

        bool isDynamic;
        static void* lastAllocAddress;

        template <typename SocketConnectorT>
        friend class SocketClient;
    };

    template <typename SocketConnectionT>
    void* SocketConnector<SocketConnectionT>::lastAllocAddress = nullptr;

} // namespace net::socket::stream

#endif // NET_SOCKET_SOCK_STREAM_SOCKETCONNECTOR_H
