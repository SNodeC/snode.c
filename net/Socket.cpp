#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h>

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Socket.h"
#include "Descriptor.h"


Socket::Socket() {}


Socket::~Socket() {
    ::shutdown(this->getFd(), SHUT_RDWR);
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
