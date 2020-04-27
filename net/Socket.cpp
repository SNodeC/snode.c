#include <sys/socket.h>

#include "Socket.h"
#include "Descriptor.h"


Socket::Socket() {
    this->setFd(::socket(AF_INET, SOCK_STREAM, 0));
}


Socket::~Socket() {
    if (this->getFd() != 0) {
        ::shutdown(this->getFd(), SHUT_RDWR);
    }
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
