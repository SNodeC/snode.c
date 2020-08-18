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

#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <easylogging++.h>
#include <functional>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AcceptEventReceiver.h"
#include "Logger.h"
#include "ReadEventReceiver.h"
#include "Socket.h"

template <typename SocketConnection>
class SocketServer
    : public AcceptEventReceiver
    , public Socket {
public:
    void* operator new(size_t size) {
        SocketServer* ss = reinterpret_cast<SocketServer*>(malloc(size));
        ss->isDynamic = true;

        return ss;
    }

    void operator delete(void* ss_v) {
        SocketServer* ss = reinterpret_cast<SocketServer*>(ss_v);
        if (ss->isDynamic) {
            free(ss_v);
        }
    }

    SocketServer() = delete;

    SocketServer(const std::function<void(SocketConnection* cs)>& onConnect, const std::function<void(SocketConnection* cs)>& onDisconnect,
                 const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                 const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                 const std::function<void(SocketConnection* cs, int errnum)>& onWriteError)
        : AcceptEventReceiver()
        , Socket()
        , onConnect(onConnect)
        , onDisconnect(onDisconnect)
        , onRead(onRead)
        , onReadError(onReadError)
        , onWriteError(onWriteError) {
    }

    ~SocketServer() override = default;

    SocketServer(const SocketServer&) = delete;
    SocketServer& operator=(const SocketServer&) = delete;

    void listen(const InetAddress& localAddress, int backlog, const std::function<void(int err)>& onError) {
        this->open([this, &localAddress, &backlog, &onError](int errnum) -> void {
            if (errnum > 0) {
                onError(errnum);
            } else {
                this->reuseAddress([this, &localAddress, &backlog, &onError](int errnum) -> void {
                    if (errnum != 0) {
                        onError(errnum);
                    } else {
                        this->bind(localAddress, [this, &backlog, &onError](int errnum) -> void {
                            if (errnum > 0) {
                                onError(errnum);
                            } else {
                                this->listen(backlog, [this, &onError](int errnum) -> void {
                                    if (errnum == 0) {
                                        AcceptEventReceiver::enable();
                                    }
                                    onError(errnum);
                                });
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

    void acceptEvent() override {
        errno = 0;

        struct sockaddr_in remoteAddress {};
        socklen_t addrlen = sizeof(remoteAddress);

        int csFd = -1;

        csFd = ::accept4(this->getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &addrlen, SOCK_NONBLOCK);

        if (csFd >= 0) {
            struct sockaddr_in localAddress {};
            socklen_t addressLength = sizeof(localAddress);

            if (getsockname(csFd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                SocketConnection* cs = SocketConnection::create(csFd, onRead, onReadError, onWriteError, onDisconnect);

                cs->setRemoteAddress(InetAddress(remoteAddress));
                cs->setLocalAddress(InetAddress(localAddress));

                cs->ReadEventReceiver::enable();

                onConnect(cs);
            } else {
                PLOG(ERROR) << "getsockname";
                shutdown(csFd, SHUT_RDWR);
                ::close(csFd);
            }
        } else if (errno != EINTR) {
            PLOG(ERROR) << "accept";
        }
    }

    void end() {
        AcceptEventReceiver::disable();
    }

protected:
    void listen(int backlog, const std::function<void(int errnum)>& onError) {
        int ret = ::listen(this->getFd(), backlog);

        if (ret < 0) {
            onError(errno);
        } else {
            onError(0);
        }
    }

private:
    void reuseAddress(const std::function<void(int errnum)>& onError) {
        int sockopt = 1;

        if (setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
            onError(errno);
        } else {
            onError(0);
        }
    }

    void unobserved() override {
        if (isDynamic) {
            delete this;
        }
    }

    std::function<void(SocketConnection* cs)> onConnect;
    std::function<void(SocketConnection* cs)> onDisconnect;
    std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> onRead;
    std::function<void(SocketConnection* cs, int errnum)> onReadError;
    std::function<void(SocketConnection* cs, int errnum)> onWriteError;

    bool isDynamic;

public:
    //    using SocketConnectionType = SocketConnectionImpl;
};

#endif // SOCKETSERVER_H
