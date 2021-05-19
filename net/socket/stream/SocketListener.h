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

#ifndef NET_SOCKET_STREAM_SOCKETLISTENER_H
#define NET_SOCKET_STREAM_SOCKETLISTENER_H

#include "log/Logger.h"
#include "net/AcceptEventReceiver.h"
#include "net/ReadEventReceiver.h"
#include "net/socket/stream/SocketProtocolFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketConnectionT>
    class SocketListener
        : public AcceptEventReceiver
        , public SocketConnectionT::Socket {
    public:
        using SocketConnection = SocketConnectionT;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        SocketListener(const std::shared_ptr<const SocketProtocolFactory>& socketProtocolFactory,
                       const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                       const std::function<void(SocketConnection* socketConnection)>& onConnected,
                       const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : socketProtocolFactory(socketProtocolFactory)
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        SocketListener() = delete;
        SocketListener(const SocketListener&) = delete;

        SocketListener& operator=(const SocketListener&) = delete;

        virtual ~SocketListener() = default;

        void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int err)>& onError) {
            errno = 0;

            Socket::open([this, &bindAddress, &backlog, &onError](int errnum) -> void {
                if (errnum > 0) {
                    onError(errnum);
                    destruct();
                } else {
                    reuseAddress([this, &bindAddress, &backlog, &onError](int errnum) -> void {
                        if (errnum != 0) {
                            onError(errnum);
                            destruct();
                        } else {
                            Socket::bind(bindAddress, [this, &backlog, &onError](int errnum) -> void {
                                if (errnum > 0) {
                                    onError(errnum);
                                    destruct();
                                } else {
                                    int ret = ::listen(Socket::getFd(), backlog);

                                    if (ret == 0) {
                                        AcceptEventReceiver::enable(Socket::getFd());
                                        onError(0);
                                    } else {
                                        onError(errno);
                                        destruct();
                                    }
                                }
                            });
                        }
                    });
                }
            });
        }

    private:
        void reuseAddress(const std::function<void(int errnum)>& onError) {
            int sockopt = 1;

            if (setsockopt(Socket::getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                onError(0);
            }
        }

        void acceptEvent() override {
            errno = 0;

            typename SocketAddress::SockAddr remoteAddress{};
            socklen_t remoteAddressLength = sizeof(remoteAddress);

            int fd = -1;

            fd = ::accept4(Socket::getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &remoteAddressLength, SOCK_NONBLOCK);

            if (fd >= 0) {
                typename SocketAddress::SockAddr localAddress{};
                socklen_t addressLength = sizeof(localAddress);

                if (getsockname(fd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                    SocketConnection* socketConnection = new SocketConnection(
                        socketProtocolFactory, fd, SocketAddress(localAddress), SocketAddress(remoteAddress), onConnect, onDisconnect);

                    onConnected(socketConnection);
                } else {
                    PLOG(ERROR) << "getsockname";
                    shutdown(fd, SHUT_RDWR);
                    ::close(fd);
                }
            } else if (errno != EINTR) {
                PLOG(ERROR) << "accept";
            }
        }

        void end() {
            AcceptEventReceiver::disable(Socket::getFd());
        }

        void unobserved() override {
            destruct();
        }

        void destruct() {
            delete this;
        }

        std::shared_ptr<const SocketProtocolFactory> socketProtocolFactory = nullptr;

        std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onDestruct;
        std::function<void(SocketConnection* socketConnection)> onConnected;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETLISTENER_H
