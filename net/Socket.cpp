#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h>

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Socket.h"
#include "Descriptor.h"


Socket::Socket() {}


Socket::Socket(int fd) : SocketBase() {
    this->setFd(fd);
}


Socket::~Socket() {
}


void Socket::setFd(int fd) {
    SocketBase::setFd(fd);
}


ssize_t Socket::recv(void *buf, size_t len, int flags) {
    return ::recv(this->getFd(), buf, len, flags);
}


ssize_t Socket::send(const void *buf, size_t len, int flags) {
    return ::send(this->getFd(), buf, len, flags);
}
