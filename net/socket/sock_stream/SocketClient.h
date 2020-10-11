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

#ifndef NET_SOCKET_SOCK_STREAM_SOCKETCLIENT_H
#define NET_SOCKET_SOCK_STREAM_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <functional>
#include <map>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "Logger.h"
#include "ReadEventReceiver.h"
#include "WriteEventReceiver.h"
#include "timer/SingleshotTimer.h"

#define CONNECT_TIMEOUT 10

namespace net::socket::stream {

    template <typename SocketConnectionT>
    class SocketClient {
    public:
        using SocketConnection = SocketConnectionT;

        SocketClient(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
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
            , options(options) {
        }

        SocketClient() = delete;

        virtual ~SocketClient() = default;

    public:
        // NOLINTNEXTLINE(google-default-arguments)
        virtual void
        connect(const std::map<std::string, std::any>& options,
                const std::function<void(int err)>& onError,
                const typename SocketConnection::Socket::SocketAddress& localAddress = typename SocketConnection::Socket::SocketAddress()) {
            std::string host = "";
            unsigned short port = 0;

            for (auto& [name, value] : options) {
                if (name == "host") {
                    host = std::any_cast<const char*>(value);
                } else if (name == "port") {
                    port = std::any_cast<int>(value);
                }
            }

            SocketConnection* socketConnection =
                new SocketConnection(onConstruct, onDestruct, onRead, onReadError, onWriteError, onDisconnect);

            socketConnection->open(
                [socketConnection, &host, &port, &localAddress, &onConnect = this->onConnect, &onError](int err) -> void {
                    if (err) {
                        onError(err);
                        delete socketConnection;
                    } else {
                        socketConnection->bind(localAddress, [socketConnection, &host, &port, &onConnect, &onError](int err) -> void {
                            if (err) {
                                onError(err);
                                delete socketConnection;
                            } else {
                                typename SocketConnection::Socket::SocketAddress server(host, port);

                                class Connector
                                    : public WriteEventReceiver
                                    , public Descriptor {
                                public:
                                    Connector(SocketConnection* socketConnection,
                                              const typename SocketConnection::Socket::SocketAddress& server,
                                              const std::function<void(SocketConnection* socketConnection)>& onConnect,
                                              const std::function<void(int err)>& onError)
                                        : socketConnection(socketConnection)
                                        , onConnect(onConnect)
                                        , onError(onError)
                                        , timeOut(net::timer::Timer::singleshotTimer(
                                              [this, socketConnection, onError](const void*) -> void {
                                                  onError(ETIMEDOUT);
                                                  WriteEventReceiver::disable();
                                                  delete socketConnection;
                                              },
                                              (struct timeval){CONNECT_TIMEOUT, 0},
                                              nullptr)) {
                                        open(socketConnection->getFd(), FLAGS::dontClose);
                                        errno = 0;
                                        int ret = ::connect(socketConnection->getFd(), &server.getSockAddr(), server.getSockAddrLen());

                                        if (ret == 0 || errno == EINPROGRESS) {
                                            typename SocketConnection::Socket::SocketAddress::SockAddr localAddress{};
                                            socklen_t addressLength = sizeof(localAddress);
                                            getsockname(
                                                socketConnection->getFd(), reinterpret_cast<sockaddr*>(&localAddress), &addressLength);
                                            socketConnection->setLocalAddress(
                                                typename SocketConnection::Socket::SocketAddress(localAddress));
                                            socketConnection->setRemoteAddress(server);
                                        }

                                        if (ret == 0) {
                                            timeOut.cancel();

                                            socketConnection->ReadEventReceiver::enable();

                                            onError(0);
                                            onConnect(socketConnection);

                                            delete this;
                                        } else if (errno == EINPROGRESS) {
                                            WriteEventReceiver::enable();
                                        } else {
                                            timeOut.cancel();
                                            onError(errno);
                                            delete socketConnection;
                                            delete this;
                                        }
                                    }

                                    void writeEvent() override {
                                        int cErrno = 0;
                                        socklen_t cErrnoLen = sizeof(cErrno);

                                        int err = getsockopt(socketConnection->getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

                                        if (cErrno != EINPROGRESS) {
                                            timeOut.cancel();

                                            WriteEventReceiver::disable();

                                            if (err < 0) {
                                                delete socketConnection;
                                                onError(errno);
                                            } else if (cErrno != 0) {
                                                delete socketConnection;
                                                errno = cErrno;
                                                onError(cErrno);
                                            } else {
                                                socketConnection->ReadEventReceiver::enable();

                                                onError(0);
                                                onConnect(socketConnection);
                                            }
                                        }
                                    }

                                    void unobserved() override {
                                        delete this;
                                    }

                                private:
                                    SocketConnection* socketConnection = nullptr;
                                    std::function<void(SocketConnection* socketConnection)> onConnect;
                                    std::function<void(int err)> onError;
                                    net::timer::Timer& timeOut;
                                };

                                errno = 0;

                                new Connector(socketConnection, server, onConnect, onError);
                            }
                        });
                    }
                },
                SOCK_NONBLOCK);
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
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_SOCK_STREAM_SOCKETCLIENT_H
