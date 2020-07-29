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
#include "Writer.h"
#include "timer/SingleshotTimer.h"

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
        SocketConnectionImpl* cs = SocketConnectionImpl::create(onRead, onReadError, onWriteError, onDisconnect);

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

                            class Connect
                                : public Writer
                                , public Socket {
                            public:
                                Connect(SocketConnectionImpl* cs, InetAddress server,
                                        const std::function<void(SocketConnectionImpl* cs)>& onConnect,
                                        const std::function<void(int err)>& onError)
                                    : Descriptor(true)
                                    , cs(cs)
                                    , server(server)
                                    , onConnect(onConnect)
                                    , onError(onError)
                                    , timeOut(Timer::singleshotTimer(
                                          [this]([[maybe_unused]] const void* arg) -> void {
                                              this->onError(ETIMEDOUT);
                                              this->::Writer::stop();
                                              delete this->cs;
                                          },
                                          (struct timeval){10, 0}, nullptr)) {
                                    this->attachFd(cs->getFd());

                                    errno = 0;
                                    int ret = ::connect(cs->getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()),
                                                        sizeof(server.getSockAddr()));

                                    if (ret == 0) {
                                        timeOut.cancel();
                                        onConnect(cs);
                                        cs->::Reader::start();
                                        delete this;
                                    } else {
                                        if (errno == EINPROGRESS) {
                                            ::Writer::start();
                                        } else {
                                            timeOut.cancel();
                                            onError(errno);
                                            delete cs;
                                            delete this;
                                        }
                                    }
                                }

                                void writeEvent() override {
                                    int cErrno;
                                    int err;
                                    socklen_t cErrnoLen = sizeof(cErrno);

                                    err = getsockopt(cs->getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

                                    timeOut.cancel();
                                    ::Writer::stop();

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
                                        cs->setRemoteAddress(server);
                                        cs->setLocalAddress(InetAddress(localAddress));

                                        onError(0);
                                        onConnect(cs);
                                        cs->::Reader::start();
                                    }
                                }

                                void unmanaged() override {
                                    delete this;
                                }

                            private:
                                SocketConnectionImpl* cs = nullptr;
                                InetAddress server;
                                std::function<void(SocketConnectionImpl* cs)> onConnect;
                                std::function<void(int err)> onError;
                                Timer& timeOut;
                            };

                            new Connect(cs, server, onConnect, onError);
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
