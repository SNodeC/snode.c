#include <sys/socket.h>

#include "Socket.h"
#include "Descriptor.h"


Socket::Socket() {}


Socket::~Socket() {
    ::shutdown(this->getFd(), SHUT_RDWR);
}

void Socket::open() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    this->setFd(fd);
}

InetAddress& Socket::getLocalAddress() {
    return localAddress;
}


int Socket::bind(InetAddress& localAddress) {
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    return ::bind(this->getFd(), (struct sockaddr*) &localAddress.getSockAddr(), addrlen);
}


void Socket::setLocalAddress(const InetAddress& localAddress) {
    this->localAddress = localAddress;
}
