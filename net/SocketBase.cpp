#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketBase.h"


SocketBase::SocketBase() {}


SocketBase::~SocketBase() {
    ::shutdown(this->getFd(), SHUT_RDWR);
}


void SocketBase::setFd(int fd) {
    Descriptor::setFd(fd);
}


void SocketBase::open(const std::function<void (int errnum)>& onError) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (fd >= 0) {
        this->setFd(fd);
        onError(0);
    } else {
        onError(errno);
    }
}


InetAddress& SocketBase::getLocalAddress() {
    return localAddress;
}


void SocketBase::bind(InetAddress& localAddress, const std::function<void (int errnum)>& onError) {
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int ret = ::bind(this->getFd(), (struct sockaddr*) &localAddress.getSockAddr(), addrlen);

    if (ret < 0) {
        onError(errno);
    } else {
        onError(0);
    }
}


void SocketBase::listen(int backlog, const std::function<void (int errnum)>& onError) {
    int ret = ::listen(this->getFd(), backlog);

    if (ret < 0) {
        onError(errno);
    } else {
        onError(0);
    }
}


void SocketBase::setLocalAddress(const InetAddress& localAddress) {
    this->localAddress = localAddress;
}
