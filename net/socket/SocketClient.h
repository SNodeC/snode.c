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

#ifndef SOCKETCLIENTBASE_H
#define SOCKETCLIENTBASE_H

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

template <typename SocketConnection>
class SocketClient {
public:
    SocketClient(const std::function<void(SocketConnection* cs)>& onConnect, const std::function<void(SocketConnection* cs)>& onDisconnect,
                 const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                 const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                 const std::function<void(SocketConnection* cs, int errnum)>& onWriteError)
        : onConnect(onConnect)
        , onDisconnect(onDisconnect)
        , onRead(onRead)
        , onReadError(onReadError)
        , onWriteError(onWriteError) {
    }

    // NOLINTNEXTLINE(google-default-arguments)
    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                         const InetAddress& localAddress = InetAddress()) {
        SocketConnection* cs = SocketConnection::create(onRead, onReadError, onWriteError, onDisconnect);

        cs->open(
            [this, &cs, &host, &port, &localAddress, &onError](int err) -> void {
                if (err) {
                    onError(err);
                    delete cs;
                } else {
                    cs->bind(localAddress, [this, &cs, &host, &port, &onError](int err) -> void {
                        if (err) {
                            onError(err);
                            delete cs;
                        } else {
                            InetAddress server(host, port);
                            errno = 0;

                            class Connector
                                : public WriteEventReceiver
                                , public Socket {
                            public:
                                Connector(SocketConnection* cs, const InetAddress& server,
                                          const std::function<void(SocketConnection* cs)>& onConnect,
                                          const std::function<void(int err)>& onError)
                                    : Descriptor(true)
                                    , cs(cs)
                                    , server(server)
                                    , onConnect(onConnect)
                                    , onError(onError)
                                    , timeOut(Timer::singleshotTimer(
                                          [this]([[maybe_unused]] const void* arg) -> void {
                                              this->onError(ETIMEDOUT);
                                              this->WriteEventReceiver::disable();
                                              delete this->cs;
                                          },
                                          (struct timeval){CONNECT_TIMEOUT, 0}, nullptr)) {
                                    this->attachFd(cs->getFd());

                                    errno = 0;
                                    int ret = ::connect(cs->getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()),
                                                        sizeof(server.getSockAddr()));

                                    if (ret == 0) {
                                        timeOut.cancel();

                                        struct sockaddr_in localAddress {};
                                        socklen_t addressLength = sizeof(localAddress);
                                        getsockname(cs->getFd(), reinterpret_cast<sockaddr*>(&localAddress), &addressLength);
                                        cs->setLocalAddress(InetAddress(localAddress));
                                        cs->setRemoteAddress(server);

                                        cs->ReadEventReceiver::enable();

                                        onError(0);
                                        onConnect(cs);
                                        delete this;
                                    } else {
                                        if (errno == EINPROGRESS) {
                                            this->WriteEventReceiver::enable();
                                        } else {
                                            timeOut.cancel();
                                            onError(errno);
                                            delete cs;
                                            delete this;
                                        }
                                    }
                                }

                                void writeEvent() override {
                                    int cErrno = 0;
                                    socklen_t cErrnoLen = sizeof(cErrno);

                                    int err = getsockopt(cs->getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

                                    timeOut.cancel();
                                    this->WriteEventReceiver::disable();

                                    if (err < 0) {
                                        onError(err);
                                        delete cs;
                                    } else if (cErrno != 0) {
                                        onError(cErrno);
                                        delete cs;
                                    } else {
                                        struct sockaddr_in localAddress {};
                                        socklen_t addressLength = sizeof(localAddress);
                                        getsockname(cs->getFd(), reinterpret_cast<sockaddr*>(&localAddress), &addressLength);
                                        cs->setLocalAddress(InetAddress(localAddress));
                                        cs->setRemoteAddress(server);

                                        cs->ReadEventReceiver::enable();

                                        onError(0);
                                        onConnect(cs);
                                    }
                                }

                                void unobserved() override {
                                    delete this;
                                }

                            private:
                                SocketConnection* cs = nullptr;
                                InetAddress server;
                                std::function<void(SocketConnection* cs)> onConnect;
                                std::function<void(int err)> onError;
                                Timer& timeOut;
                            };

                            new Connector(cs, server, onConnect, onError);
                        }
                    });
                }
            },
            SOCK_NONBLOCK);
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, in_port_t lPort) {
        connect(host, port, onError, InetAddress(lPort));
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string& lHost) {
        connect(host, port, onError, InetAddress(lHost));
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string& lHost,
                         in_port_t lPort) {
        connect(host, port, onError, InetAddress(lHost, lPort));
    }

protected:
    virtual ~SocketClient() = default;

private:
    std::function<void(SocketConnection* cs)> onConnect;
    std::function<void(SocketConnection* cs)> onDisconnect;
    std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> onRead;
    std::function<void(SocketConnection* cs, int errnum)> onReadError;
    std::function<void(SocketConnection* cs, int errnum)> onWriteError;
};

#endif // SOCKETCLIENTBASE_H
