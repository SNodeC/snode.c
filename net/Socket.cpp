#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h>

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Socket.h"
#include "Descriptor.h"


Socket::Socket() {}


Socket::Socket(int fd) : Descriptor(fd) {
    this->setFd(fd);
}


Socket::~Socket() {
    ::shutdown(this->getFd(), SHUT_RDWR);
}


void Socket::setFd(int fd) {
    Descriptor::setFd(fd);
}


void Socket::open(const std::function<void (int errnum)>& onError) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    
    if (fd >= 0) {
        this->setFd(fd);
        onError(0);
    } else {
        onError(errno);
    }
}


ssize_t Socket::recv(void *buf, size_t len, int flags) {
    return ::recv(this->getFd(), buf, len, flags);
}


ssize_t Socket::send(const void *buf, size_t len, int flags) {
    return ::send(this->getFd(), buf, len, flags);
}


InetAddress& Socket::getLocalAddress() {
    return localAddress;
}


void Socket::bind(InetAddress& localAddress, const std::function<void (int errnum)>& onError) {
    int ret = 0;
    
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    ret = ::bind(this->getFd(), (struct sockaddr*) &localAddress.getSockAddr(), addrlen);
    
    if (ret < 0) {
        onError(errno);
    } else {
        onError(0);
    }
}


void Socket::setLocalAddress(const InetAddress& localAddress) {
    this->localAddress = localAddress;
}
