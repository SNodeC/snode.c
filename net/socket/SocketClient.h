#ifndef SOCKETCLIENTBASE_H
#define SOCKETCLIENTBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Reader.h"
#include "Socket.h"
#include "timer/IntervalTimer.h"


template <typename SocketConnectionImpl>
class SocketClient {
public:
    SocketClient(const std::function<void(SocketConnectionImpl* cs)>& onConnect,
                 const std::function<void(SocketConnectionImpl* cs)>& onDisconnect,
                 const std::function<void(SocketConnectionImpl* cs, const char* junk, ssize_t n)>& onRead,
                 const std::function<void(SocketConnectionImpl* cs, int errnum)>& onReadError,
                 const std::function<void(SocketConnectionImpl* cs, int errnum)>& onWriteError)
        : onConnect(onConnect)
        , onDisconnect(onDisconnect)
        , onRead(onRead)
        , onReadError(onReadError)
        , onWriteError(onWriteError) {
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                         const InetAddress& localAddress = InetAddress()) {
        SocketConnectionImpl* cs = new SocketConnectionImpl(onRead, onReadError, onWriteError,
                                                            [this](SocketConnectionImpl* cs) -> void { // onDisconnect
                                                                this->onDisconnect(cs);
                                                                delete cs;
                                                            });
        cs->open(
            [this, &cs, &host, &port, &localAddress, &onError](int err) -> void {
                if (err) {
                    onError(err);
                    delete cs;
                } else {
                    cs->bind(localAddress, [this, &cs, &host, &port, &onError](int err) -> void {
                        if (err) {
                            onError(err);
                        } else {
                            InetAddress server(host, port);
                            errno = 0;
                            int ret = ::connect(cs->getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()),
                                                sizeof(server.getSockAddr()));

                            [[maybe_unused]] Timer& ct = Timer::continousTimer(
                                [this, cs, server, onError]([[maybe_unused]] const void* arg, const std::function<void()>& stop) -> void {
                                    errno = 0;
                                    int ret = ::connect(cs->getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()),
                                                        sizeof(server.getSockAddr()));
                                    if (ret < 0 && errno != EINPROGRESS) {
                                        onError(errno);
                                        delete cs;
                                        stop();
                                    } else if (ret == 0) {
                                        struct sockaddr_in localAddress {};
                                        socklen_t addressLength = sizeof(localAddress);
                                        getsockname(cs->getFd(), reinterpret_cast<sockaddr*>(&localAddress), &addressLength);
                                        cs->setRemoteAddress(server);
                                        cs->setLocalAddress(InetAddress(localAddress));

                                        onError(0);
                                        onConnect(cs);
                                        cs->::Reader::start();
                                        stop();
                                    }
                                },
                                (struct timeval){0, 0}, "Connect");

                            if (ret < 0) {
                                onError(errno);
                                if (errno != EINPROGRESS) {
                                    ct.cancel();
                                    delete cs;
                                }
                            } else if (ret == 0) {
                                ct.cancel();
                                cs->::Reader::start();
                                onError(0);
                            }
                        }
                    });
                }
            },
            SOCK_NONBLOCK);
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, in_port_t lPort) {
        connect(host, port, onError, InetAddress(lPort));
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string lHost) {
        connect(host, port, onError, InetAddress(lHost));
    }

    virtual void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string lHost,
                         in_port_t lPort) {
        connect(host, port, onError, InetAddress(lHost, lPort));
    }

protected:
    virtual ~SocketClient() {
    }

private:
    std::function<void(SocketConnectionImpl* cs)> onConnect;
    std::function<void(SocketConnectionImpl* cs)> onDisconnect;
    std::function<void(SocketConnectionImpl* cs, const char* junk, ssize_t n)> onRead;
    std::function<void(SocketConnectionImpl* cs, int errnum)> onReadError;
    std::function<void(SocketConnectionImpl* cs, int errnum)> onWriteError;
};


#endif // SOCKETCLIENTBASE_H
