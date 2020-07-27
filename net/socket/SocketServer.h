#ifndef SOCKETSERVERBASE_H
#define SOCKETSERVERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Reader.h"
#include "Server.h"
#include "Socket.h"


template <typename SocketConnectionImpl>
class SocketServer
    : public Server
    , public Socket {
public:
    SocketServer(const std::function<void(SocketConnectionImpl* cs)>& onConnect,
                 const std::function<void(SocketConnectionImpl* cs)>& onDisconnect,
                 const std::function<void(SocketConnectionImpl* cs, const char* junk, ssize_t n)>& onRead,
                 const std::function<void(SocketConnectionImpl* cs, int errnum)>& onReadError,
                 const std::function<void(SocketConnectionImpl* cs, int errnum)>& onWriteError)
        : Server()
        , Socket()
        , onConnect(onConnect)
        , onDisconnect(onDisconnect)
        , onRead(onRead)
        , onReadError(onReadError)
        , onWriteError(onWriteError) {
    }

protected:
    virtual ~SocketServer() {
    }

public:
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
                                        Server::start();
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

        csFd = ::accept4(this->getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &addrlen, 0);
        
        if (csFd >= 0) {
            struct sockaddr_in localAddress {};
            socklen_t addressLength = sizeof(localAddress);

            if (getsockname(csFd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                SocketConnectionImpl* cs = new SocketConnectionImpl(csFd, onRead, onReadError, onWriteError,
                                                                    [this](SocketConnectionImpl* cs) -> void { // onDisconnect
                                                                        this->onDisconnect(cs);
                                                                        delete cs;
                                                                    });

                cs->setRemoteAddress(InetAddress(remoteAddress));
                cs->setLocalAddress(InetAddress(localAddress));
                cs->setNonBlocking();

                onConnect(cs);
                cs->::Reader::start();
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
        Server::stop();
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
    void unmanaged() override {
        delete this;
    }

    std::function<void(SocketConnectionImpl* cs)> onConnect;
    std::function<void(SocketConnectionImpl* cs)> onDisconnect;
    std::function<void(SocketConnectionImpl* cs, const char* junk, ssize_t n)> onRead;
    std::function<void(SocketConnectionImpl* cs, int errnum)> onReadError;
    std::function<void(SocketConnectionImpl* cs, int errnum)> onWriteError;

public:
    //    using SocketConnectionType = SocketConnectionImpl;
};

#endif // SOCKETSERVERBASE_H
