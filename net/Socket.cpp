#include <sys/socket.h>

#include "Socket.h"

Socket::Socket(int csFd) : Descriptor(csFd) {
}


Socket::~Socket() {
    ::shutdown(this->getFd(), SHUT_RDWR);
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
