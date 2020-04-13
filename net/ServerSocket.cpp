#include "ServerSocket.h"
#include "ConnectedSocket.h"

ServerSocket::ServerSocket(uint16_t port) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int sockopt = 1;
    setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    
    localAddress = InetAddress(port);
    this->bind(localAddress);
    this->listen(5);
}


ServerSocket::ServerSocket(const std::string hostname, uint16_t port) {
}


ServerSocket* ServerSocket::instance(uint16_t port) {
    return new ServerSocket(port);
}


ServerSocket* ServerSocket::instance(const std::string& hostname, uint16_t port) {
    return new ServerSocket(hostname, port);
}


ConnectedSocket* ServerSocket::accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    
    int csFd = ::accept(fd, (struct sockaddr*) &addr, &addrlen);
    
    if (csFd < 0) {
        return 0;
    }
    
    ConnectedSocket* cs = new ConnectedSocket(csFd, this);
    
    cs->setFd(csFd);
    cs->setRemoteAddress(InetAddress(addr));
    
    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    
    if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) < 0) {
        delete cs;
        return 0;
    }
    
    cs->setLocalAddress(InetAddress(localAddress));
    
    return cs;
}


void ServerSocket::process(Request& request, Response& response) {
    getProcessor(request, response);
}
