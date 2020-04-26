#include <sys/socket.h>

#include "Socket.h"
#include "Descriptor.h"


Socket::Socket() : fd(::socket(AF_INET, SOCK_STREAM, 0)) {}


Socket::~Socket() {
    if (fd != 0) {
        ::shutdown(fd, SHUT_RDWR);
    }
}


InetAddress& Socket::getLocalAddress() {
    return localAddress;
}


int Socket::bind(InetAddress& localAddress) {
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    return ::bind(fd, (struct sockaddr*) &localAddress.getSockAddr(), addrlen);
}


void Socket::setLocalAddress(const InetAddress& localAddress) {
    this->localAddress = localAddress;
}
