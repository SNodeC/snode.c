#ifndef SOCKETLISTENER_H
#define SOCKETLISTENER_H

#include "AcceptEventReceiver.h"
#include "ReadEventReceiver.h"
#include "Socket.h"

#include <any>
#include <easylogging++.h>
#include <functional>
#include <map>
#include <unistd.h>

namespace net::socket {

    template <typename SocketConnectionT>
    class SocketListener
        : public AcceptEventReceiver
        , public Socket {
    public:
        using SocketConnection = SocketConnectionT;

        void* operator new(size_t size) {
            SocketListener<SocketConnection>::lastAllocAddress = malloc(size);

            return SocketListener<SocketConnection>::lastAllocAddress;
        }

        void operator delete(void* socketListener_v) {
            free(socketListener_v);
        }

        SocketListener(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                       const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                       const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError)
            : AcceptEventReceiver()
            , Socket()
            , onConnect(onConnect)
            , onDisconnect(onDisconnect)
            , onRead(onRead)
            , onReadError(onReadError)
            , onWriteError(onWriteError)
            , isDynamic(this == SocketListener::lastAllocAddress) {
        }

        SocketListener() = delete;
        SocketListener(const SocketListener&) = delete;

        SocketListener& operator=(const SocketListener&) = delete;

        virtual ~SocketListener() = default;

        void reuseAddress(const std::function<void(int errnum)>& onError) {
            int sockopt = 1;

            if (setsockopt(getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                onError(0);
            }
        }

        void acceptEvent() override {
            errno = 0;

            struct sockaddr_in remoteAddress {};
            socklen_t addrlen = sizeof(remoteAddress);

            int scFd = -1;

            scFd = ::accept4(getFd(), reinterpret_cast<struct sockaddr*>(&remoteAddress), &addrlen, SOCK_NONBLOCK);

            if (scFd >= 0) {
                struct sockaddr_in localAddress {};
                socklen_t addressLength = sizeof(localAddress);

                if (getsockname(scFd, reinterpret_cast<sockaddr*>(&localAddress), &addressLength) == 0) {
                    SocketConnection* socketConnection = SocketConnection::create(onRead, onReadError, onWriteError, onDisconnect);

                    socketConnection->open(scFd);

                    socketConnection->setRemoteAddress(InetAddress(remoteAddress));
                    socketConnection->setLocalAddress(InetAddress(localAddress));

                    socketConnection->ReadEventReceiver::enable();

                    onConnect(socketConnection);
                } else {
                    PLOG(ERROR) << "getsockname";
                    shutdown(scFd, SHUT_RDWR);
                    ::close(scFd);
                }
            } else if (errno != EINTR) {
                PLOG(ERROR) << "accept";
            }
        }

        void end() {
            AcceptEventReceiver::disable();
        }

    private:
        void unobserved() override {
            if (isDynamic) {
                delete this;
            }
        }

        std::function<void(SocketConnection* socketConnection)> onConnect;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;
        std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)> onRead;
        std::function<void(SocketConnection* socketConnection, int errnum)> onReadError;
        std::function<void(SocketConnection* socketConnection, int errnum)> onWriteError;

    protected:
        bool isDynamic;
        static void* lastAllocAddress;
    };

    template <typename SocketConnection>
    void* SocketListener<SocketConnection>::lastAllocAddress = nullptr;

} // namespace net::socket

#endif // SOCKETLISTENER_H
