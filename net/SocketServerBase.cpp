#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "SocketServerBase.h"
#include "SocketLegacyConnection.h"
#include "SocketSSLConnection.h"


template<typename T>
SocketServerBase<T>::SocketServerBase(const std::function<void (SocketConnection* cs)>& onConnect,
                                      const std::function<void (SocketConnection* cs)>& onDisconnect,
                                      const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                                      const std::function<void (int errnum)>& onCsReadError,
                                      const std::function<void (int errnum)>& onCsWriteError)
    : SocketLegacyReader(), onConnect(onConnect), onDisconnect(onDisconnect), readProcessor(readProcessor), onCsReadError(onCsReadError), onCsWriteError(onCsWriteError)
{}


template<typename T>
void SocketServerBase<T>::listen(in_port_t port, int backlog, const std::function<void (int err)>& onError) {
    this->SocketLegacyReader::setOnError(onError);

    this->open([this, &port, &backlog, &onError] (int errnum) -> void {
        if (errnum > 0) {
            onError(errnum);
        } else {
            int sockopt = 1;

            if (setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                localAddress = InetAddress(port);
                this->bind(localAddress, [this, &backlog, &onError] (int errnum) -> void {
                    if (errnum > 0) {
                        onError(errnum);
                    } else {
                        this->SocketLegacy::listen(backlog, [this, &onError] (int errnum) -> void {
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


template<typename T>
void SocketServerBase<T>::readEvent() {
    struct sockaddr_in remoteAddress;
    socklen_t addrlen = sizeof(remoteAddress);

    int csFd = -1;

    csFd = ::accept(this->getFd(), (struct sockaddr*) &remoteAddress, &addrlen);

    if (csFd >= 0) {
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);

        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) == 0) {
            T* cs = new T(csFd, this, this->readProcessor, onCsReadError, onCsWriteError);

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


template<typename T>
void SocketServerBase<T>::disconnect(SocketConnection* cs) {
    if (onDisconnect) {
        onDisconnect(cs);
    }
}


template class SocketServerBase<SocketSSLConnection>;
template class SocketServerBase<SocketLegacyConnection>;
