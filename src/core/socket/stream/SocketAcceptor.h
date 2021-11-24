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

#ifndef CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_SOCKETACCEPTOR_H

#include "core/AcceptEventReceiver.h"
#include "core/system/socket.h"
#include "core/system/unistd.h"
#include "log/Logger.h"

namespace core::socket::stream {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketConnectionT>
    class SocketAcceptor
        : public SocketConnectionT::Socket
        , public AcceptEventReceiver {
        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;
        SocketAcceptor& operator=(const SocketAcceptor&) = delete;

    public:
        using SocketConnection = SocketConnectionT;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        /** Sequence diagramm of res.upgrade(req).
@startuml
!include core/socket/stream/pu/SocketAcceptor.pu!0
@enduml
*/
        SocketAcceptor(const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                       const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : socketContextFactory(socketContextFactory)
            , options(options)
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

        virtual ~SocketAcceptor() = default;

        void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int)>& onError) {
            Socket::open([this, &bindAddress, &backlog, &onError](int errnum) -> void {
                if (errnum > 0) {
                    onError(errnum);
                    destruct();
                } else {
#if !defined(NDEBUG)
                    reuseAddress([this, &bindAddress, &backlog, &onError](int errnum) -> void {
                        if (errnum != 0) {
                            onError(errnum);
                            destruct();
                        } else {
#endif
                            Socket::bind(bindAddress, [this, &backlog, &onError](int errnum) -> void {
                                if (errnum > 0) {
                                    onError(errnum);
                                    destruct();
                                } else {
                                    int ret = core::system::listen(SocketAcceptor::getFd(), backlog);

                                    if (ret == 0) {
                                        AcceptEventReceiver::enable(SocketAcceptor::getFd());
                                        onError(0);
                                    } else {
                                        onError(errno);
                                        destruct();
                                    }
                                }
                            });
#if !defined(NDEBUG)
                        }
                    });
#endif
                }
            });
        }

    private:
        void reuseAddress(const std::function<void(int)>& onError) {
            int sockopt = 1;

            if (core::system::setsockopt(SocketAcceptor::getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                onError(0);
            }
        }

        void acceptEvent() override {
            typename SocketAddress::SockAddr remoteAddress{};
            socklen_t remoteAddressLength = sizeof(remoteAddress);

            int fd = -1;

            fd = core::system::accept4(
                SocketAcceptor::getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &remoteAddressLength, SOCK_NONBLOCK);

            if (fd >= 0) {
                typename SocketAddress::SockAddr localAddress{};
                socklen_t addressLength = sizeof(localAddress);

                if (core::system::getsockname(fd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                    SocketConnection* socketConnection = new SocketConnection(
                        fd, socketContextFactory, SocketAddress(localAddress), SocketAddress(remoteAddress), onConnect, onDisconnect);

                    onConnected(socketConnection);
                } else {
                    PLOG(ERROR) << "getsockname";
                    core::system::shutdown(fd, SHUT_RDWR);
                    core::system::close(fd);
                }
            } else if (errno != EINTR) {
                PLOG(ERROR) << "accept";
            }
        }

        void end() {
            AcceptEventReceiver::disable(SocketAcceptor::getFd());
        }

        void unobservedEvent() override {
            destruct();
        }

    protected:
        void destruct() {
            delete this;
        }

    private:
        std::shared_ptr<SocketContextFactory> socketContextFactory = nullptr;

    protected:
        std::map<std::string, std::any> options;

        std::function<void(const SocketAddress&, const SocketAddress&)> onConnect;
        std::function<void(SocketConnection*)> onDestruct;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
