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


    void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError) {
        Socket socket(true);

        socket.open([this, &socket, &host, &port, &onError](int err) -> void {
            if (err) {
                onError(err);
            } else {
                // TODO: bind ...
                InetAddress server(host, port);
                errno = 0;
                int ret = ::connect(socket.getFd(), reinterpret_cast<const sockaddr*>(&server.getSockAddr()), sizeof(server.getSockAddr()));
                if (ret == 0) {
                    struct sockaddr_in localAddress {};
                    socklen_t addressLength = sizeof(localAddress);

                    if (getsockname(socket.getFd(), reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                        SocketConnectionImpl* cs = new SocketConnectionImpl(socket.getFd(), onRead, onReadError, onWriteError,
                                                                            [this](SocketConnectionImpl* cs) -> void { // onDisconnect
                                                                                this->onDisconnect(cs);
                                                                                delete cs;
                                                                            });
                        cs->setRemoteAddress(server);
                        cs->setLocalAddress(InetAddress(localAddress));

                        onConnect(cs);
                        onError(0);
                    } else {
                        int _errno = errno;
                        PLOG(ERROR) << "getsockname";
                        shutdown(socket.getFd(), SHUT_RDWR);
                        ::close(socket.getFd());
                        onError(_errno);
                    }
                } else {
                    onError(errno);
                }
            }
        });
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
