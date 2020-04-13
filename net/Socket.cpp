#include <unistd.h>
#include <sys/socket.h>

#include "Socket.h"


Socket::Socket() : fd(-1), managedCount(0) {
}


Socket::~Socket() {
    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
}


InetAddress& Socket::getLocalAddress() {
    return localAddress;
}

int Socket::getFd() const {
    return fd;
}


int Socket::bind(InetAddress& localAddress) {
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    return ::bind(fd, (struct sockaddr*) &localAddress.getSockAddr(), addrlen);
}


void Socket::setLocalAddress(const InetAddress& localAddress) {
    this->localAddress = localAddress;
}


void Socket::setFd(int fd) {
    this->fd = fd;
}


void Socket::incManagedCount() {
    managedCount++;
}

void Socket::decManagedCount() {
    managedCount--;
    
    if (managedCount == 0) {
        delete this;
    }
}
