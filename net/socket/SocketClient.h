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

#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "ReadEventReceiver.h"
#include "Socket.h"
#include "WriteEventReceiver.h"
#include "timer/SingleshotTimer.h"

#define CONNECT_TIMEOUT 10

namespace net::socket {

    template <typename SocketConnection>
    class SocketClient {
    public:
        SocketClient(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError)
            : onConnect(onConnect)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError) {
        }

        SocketClient() = delete;
        const SocketClient& operator=(const SocketClient&) = delete;

        virtual ~SocketClient() = default;

        // NOLINTNEXTLINE(google-default-arguments)
        virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                             const InetAddress& localAddress = InetAddress()) {
            SocketConnection* socketConnection = SocketConnection::create(onRead, onReadError, onWriteError, onDisconnect);

            socketConnection->open(
                [this, &socketConnection, &host, &port, &localAddress, &onError](int err) -> void {
                    if (err) {
                        onError(err);
                        delete socketConnection;
                    } else {
                        socketConnection->bind(localAddress, [this, &socketConnection, &host, &port, &onError](int err) -> void {
                            if (err) {
                                onError(err);
                                delete socketConnection;
                            } else {
                                InetAddress server(host, port);
                                errno = 0;

                                class Connector
                                    : public WriteEventReceiver
                                    , public Socket {
                                public:
                                    Connector(SocketConnection* socketConnection, const InetAddress& server,
                                              const std::function<void(SocketConnection* socketConnection)>& onConnect,
                                              const std::function<void(int err)>& onError)
                                        : Descriptor(true)
                                        , socketConnection(socketConnection)
                                        , server(server)
                                        , onConnect(onConnect)
                                        , onError(onError)
                                        , timeOut(net::timer::Timer::singleshotTimer(
                                              [this]([[maybe_unused]] const void* arg) -> void {
                                                  this->onError(ETIMEDOUT);
                                                  this->WriteEventReceiver::disable();
                                              },
                                              (struct timeval){CONNECT_TIMEOUT, 0}, nullptr)) {
                                        this->open(socketConnection->getFd());
                                        errno = 0;
                                        int ret =
                                            ::connect(socketConnection->getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()),
                                                      sizeof(server.getSockAddr()));

                                        if (ret == 0) {
                                            timeOut.cancel();

                                            struct sockaddr_in localAddress {};
                                            socklen_t addressLength = sizeof(localAddress);
                                            getsockname(socketConnection->getFd(), reinterpret_cast<sockaddr*>(&localAddress),
                                                        &addressLength);
                                            socketConnection->setLocalAddress(InetAddress(localAddress));
                                            socketConnection->setRemoteAddress(server);

                                            socketConnection->ReadEventReceiver::enable();

                                            onError(0);
                                            onConnect(socketConnection);
                                            unobserved();
                                        } else if (errno == EINPROGRESS) {
                                            WriteEventReceiver::enable();
                                        } else {
                                            timeOut.cancel();
                                            onError(errno);
                                            unobserved();
                                        }
                                    }

                                    void writeEvent() override {
                                        int cErrno = 0;
                                        socklen_t cErrnoLen = sizeof(cErrno);

                                        int err = getsockopt(socketConnection->getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

                                        timeOut.cancel();
                                        WriteEventReceiver::disable();

                                        if (err < 0) {
                                            onError(err);
                                        } else if (cErrno != 0) {
                                            onError(cErrno);
                                        } else {
                                            struct sockaddr_in localAddress {};
                                            socklen_t addressLength = sizeof(localAddress);
                                            getsockname(socketConnection->getFd(), reinterpret_cast<sockaddr*>(&localAddress),
                                                        &addressLength);
                                            socketConnection->setLocalAddress(InetAddress(localAddress));
                                            socketConnection->setRemoteAddress(server);

                                            socketConnection->ReadEventReceiver::enable();

                                            onError(0);
                                            onConnect(socketConnection);
                                        }
                                    }

                                    void unobserved() override {
                                        if (!socketConnection->isObserved()) {
                                            delete socketConnection;
                                        }
                                        delete this;
                                    }

                                private:
                                    SocketConnection* socketConnection = nullptr;
                                    InetAddress server;
                                    std::function<void(SocketConnection* socketConnection)> onConnect;
                                    std::function<void(int err)> onError;
                                    net::timer::Timer& timeOut;
                                };

                                new Connector(socketConnection, server, onConnect, onError);
                            }
                        });
                    }
                },
                SOCK_NONBLOCK);
        }

        virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, in_port_t lPort) {
            connect(host, port, onError, InetAddress(lPort));
        }

        virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                             const std::string& lHost) {
            connect(host, port, onError, InetAddress(lHost));
        }

        virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string& lHost,
                             in_port_t lPort) {
            connect(host, port, onError, InetAddress(lHost, lPort));
        }

    private:
        std::function<void(SocketConnection* socketConnection)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t n)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;
    };

} // namespace net::socket

#endif // SOCKETCLIENT_H
