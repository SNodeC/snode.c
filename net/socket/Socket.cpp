#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Socket.h"


Socket::~Socket() {
    ::shutdown(this->fd(), SHUT_RDWR);
}


void Socket::open(const std::function<void(int errnum)>& onError) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (fd >= 0) {
        this->attachFd(fd);
        onError(0);
    } else {
        onError(errno);
    }
}


InetAddress& Socket::getLocalAddress() {
    return localAddress;
}


void Socket::bind(const InetAddress& localAddress, const std::function<void(int errnum)>& onError) {
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int ret = ::bind(this->fd(), reinterpret_cast<const struct sockaddr*>(&localAddress.getSockAddr()), addrlen);

    if (ret < 0) {
        onError(errno);
    } else {
        onError(0);
    }
}


void Socket::setLocalAddress(const InetAddress& localAddress) {
    this->localAddress = localAddress;
}
