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

#ifndef CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_SOCKETACCEPTOR_H

#include "core/AcceptEventReceiver.h"

namespace core::socket {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"
#include "core/system/unistd.h"
#include "log/Logger.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_ACCEPTS_PER_TICK 20

namespace core::socket::stream {

    template <typename ServerConfigT, typename SocketConnectionT>
    class SocketAcceptor
        : protected SocketConnectionT::Socket
        , protected AcceptEventReceiver {
        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;
        SocketAcceptor& operator=(const SocketAcceptor&) = delete;

    public:
        using ServerConfig = ServerConfigT;
        using SocketConnection = SocketConnectionT;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        /** Sequence diagramm of res.upgrade(req).
@startuml
!include core/socket/stream/pu/SocketAcceptor.pu!0
@enduml
*/
        SocketAcceptor(const ServerConfig& serverConfig,
                       const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : serverConfig(serverConfig)
            , socketContextFactory(socketContextFactory)
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        virtual ~SocketAcceptor() = default;

        void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(const Socket& socket, int)>& onError) {
            Socket::open(
                [this, &bindAddress, &backlog, &onError](int errnum) -> void {
                    if (errnum > 0) {
                        onError(static_cast<const Socket&>(*this), errnum);
                        destruct();
                    } else {
#if !defined(NDEBUG)
                        reuseAddress([this, &bindAddress, &backlog, &onError](int errnum) -> void {
                            if (errnum != 0) {
                                onError(static_cast<const Socket&>(*this), errnum);
                                destruct();
                            } else {
#endif
                                Socket::bind(bindAddress, [this, &backlog, &onError](int errnum) -> void {
                                    if (errnum > 0) {
                                        onError(static_cast<const Socket&>(*this), errnum);
                                        destruct();
                                    } else {
                                        int ret = core::system::listen(Socket::fd, backlog);

                                        if (ret == 0) {
                                            enable(Socket::fd);
                                            onError(static_cast<const Socket&>(*this), 0);
                                        } else {
                                            onError(static_cast<const Socket&>(*this), errno);
                                            destruct();
                                        }
                                    }
                                });
#if !defined(NDEBUG)
                            }
                        });
#endif
                    }
                },
                SOCK_NONBLOCK);
        }

    private:
        void reuseAddress(const std::function<void(int)>& onError) {
            int sockopt = 1;

            if (core::system::setsockopt(Socket::fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                onError(0);
            }
        }

        void acceptEvent() override {
            typename SocketAddress::SockAddr remoteAddress{};
            socklen_t remoteAddressLength = sizeof(remoteAddress);

            int fd = -1;

            int count = MAX_ACCEPTS_PER_TICK;

            do {
                fd = core::system::accept4(
                    Socket::fd, reinterpret_cast<struct sockaddr*>(&remoteAddress), &remoteAddressLength, SOCK_NONBLOCK);

                if (fd >= 0) {
                    typename SocketAddress::SockAddr localAddress{};
                    socklen_t addressLength = sizeof(localAddress);

                    if (core::system::getsockname(fd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                        SocketConnection* socketConnection = new SocketConnection(
                            fd, socketContextFactory, SocketAddress(localAddress), SocketAddress(remoteAddress), onConnect, onDisconnect);

                        socketConnection->SocketReader::setTimeout(serverConfig.getReadTimeout());
                        socketConnection->SocketWriter::setTimeout(serverConfig.getWriteTimeout());
                        socketConnection->SocketReader::setBlockSize(serverConfig.getReadBlockSize());
                        socketConnection->SocketWriter::setBlockSize(serverConfig.getWriteBlockSize());

                        onConnected(socketConnection);
                    } else {
                        PLOG(ERROR) << "getsockname";
                        core::system::shutdown(fd, SHUT_RDWR);
                        core::system::close(fd);
                    }
                }
            } while (fd >= 0 && --count > 0);

            if (fd < 0 && errno != EINTR && errno != EAGAIN) {
                PLOG(ERROR) << "accept";
            }
        }

        void unobservedEvent() override {
            destruct();
        }

    protected:
        void destruct() {
            delete this;
        }

        ServerConfig serverConfig;

    private:
        std::shared_ptr<core::socket::SocketContextFactory> socketContextFactory = nullptr;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onDestruct;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

    protected:
        std::map<std::string, std::any> options;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
