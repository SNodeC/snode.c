#ifndef SOCKETCLIENTBASE_H
#define SOCKETCLIENTBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Socket.h"


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


    void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                 const InetAddress& localAddress = InetAddress()) {
        SocketConnectionImpl* cs = new SocketConnectionImpl(onRead, onReadError, onWriteError,
                                                            [this](SocketConnectionImpl* cs) -> void { // onDisconnect
                                                                this->onDisconnect(cs);
                                                                delete cs;
                                                            });
        cs->open([this, &cs, &host, &port, &localAddress, &onError](int err) -> void {
            if (err) {
                delete cs;
                onError(err);
            } else {
                cs->bind(localAddress, [this, &cs, &host, &port, &onError](int err) -> void {
                    if (err) {
                        delete cs;
                        onError(err);
                    } else {
                        errno = 0;
                        InetAddress server(host, port);
                        int ret =
                            ::connect(cs->getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()), sizeof(server.getSockAddr()));
                        if (ret == 0) {
                            struct sockaddr_in localAddress {};
                            socklen_t addressLength = sizeof(localAddress);
                            if (getsockname(cs->getFd(), reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                                cs->setRemoteAddress(server);
                                cs->setLocalAddress(InetAddress(localAddress));

                                onConnect(cs);
                                onError(0);
                            } else {
                                int _errno = errno;
                                PLOG(ERROR) << "getsockname";
                                delete cs;
                                onError(_errno);
                            }
                        } else {
                            delete cs;
                            onError(errno);
                        }
                    }
                });
            }
        });
    }

    void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, in_port_t lPort) {
        connect(host, port, onError, InetAddress(lPort));
    }

    void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string lHost) {
        connect(host, port, onError, InetAddress(lHost));
    }

    void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string lHost,
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
