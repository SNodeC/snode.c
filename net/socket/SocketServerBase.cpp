#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "SocketServerBase.h"
#include "socket/legacy/SocketConnection.h"
#include "socket/tls/SocketConnection.h"


template <typename SocketSonnectionImpl>
SocketServerBase<SocketSonnectionImpl>::SocketServerBase(
    const std::function<void(SocketSonnectionImpl* cs)>& onConnect, const std::function<void(SocketSonnectionImpl* cs)>& onDisconnect,
    const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
    const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
    const std::function<void(SocketConnection* cs, int errnum)>& onWriteError)
    : SocketReader()
    , onConnect(onConnect)
    , onDisconnect(onDisconnect)
    , readProcessor(readProcessor)
    , onReadError(onReadError)
    , onWriteError(onWriteError) {
}


template <typename SocketSonnectionImpl>
void SocketServerBase<SocketSonnectionImpl>::listen(int backlog, const std::function<void(int errnum)>& onError) {
    int ret = ::listen(this->fd(), backlog);

    if (ret < 0) {
        onError(errno);
    } else {
        onError(0);
    }
}


template <typename SocketSonnectionImpl>
void SocketServerBase<SocketSonnectionImpl>::listen(in_port_t port, int backlog, const std::function<void(int err)>& onError) {
    this->SocketReader::setOnError(onError);

    this->open([this, &port, &backlog, &onError](int errnum) -> void {
        if (errnum > 0) {
            onError(errnum);
        } else {
            int sockopt = 1;
            if (setsockopt(this->fd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                localAddress = InetAddress(port);
                this->bind(localAddress, [this, &backlog, &onError](int errnum) -> void {
                    if (errnum > 0) {
                        onError(errnum);
                    } else {
                        this->listen(backlog, [this, &onError](int errnum) -> void {
                            if (errnum == 0) {
                                Multiplexer::instance().getManagedReader().add(this);
                            }
                            onError(errnum);
                        });
                    }
                });
            }
        }
    });
}


template <typename SocketSonnectionImpl>
void SocketServerBase<SocketSonnectionImpl>::readEvent() {
    errno = 0;

    struct sockaddr_in remoteAddress;
    socklen_t addrlen = sizeof(remoteAddress);

    int csFd = -1;

    csFd = ::accept(this->fd(), (struct sockaddr*) &remoteAddress, &addrlen);

    if (csFd >= 0) {
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);

        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) == 0) {
            SocketSonnectionImpl* cs = new SocketSonnectionImpl(csFd, this, readProcessor, onReadError, onWriteError);

            cs->setRemoteAddress(remoteAddress);
            cs->setLocalAddress(localAddress);

            Multiplexer::instance().getManagedReader().add(cs);
            onConnect(cs);
        } else {
            shutdown(csFd, SHUT_RDWR);
            close(csFd);
            onError(errno);
        }
    } else if (errno != EINTR) {
        onError(errno);
    }
}


template <typename SocketSonnectionImpl>
void SocketServerBase<SocketSonnectionImpl>::disconnect(SocketConnection* cs) {
    if (onDisconnect) {
        onDisconnect(dynamic_cast<SocketSonnectionImpl*>(cs));
    }
    delete cs;
}


template <typename SocketSonnectionImpl>
void SocketServerBase<SocketSonnectionImpl>::unmanaged() {
    delete this;
}


template class SocketServerBase<tls::SocketConnection>;
template class SocketServerBase<legacy::SocketConnection>;
