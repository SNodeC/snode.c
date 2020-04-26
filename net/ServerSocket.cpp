#include <unistd.h>       

#include "ServerSocket.h"
#include "AcceptedSocket.h"
#include "SocketMultiplexer.h"


ServerSocket::ServerSocket() : SocketReader(), rootDir(".") {
    SocketMultiplexer::instance().getReadManager().manageSocket(this);
}


ServerSocket::ServerSocket(uint16_t port) : ServerSocket() {
    int sockopt = 1;
    setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    
    localAddress = InetAddress(port);
    this->bind(localAddress);
    this->listen(5);
}


ServerSocket::ServerSocket(const std::string hostname, uint16_t port) : ServerSocket() {
}


ServerSocket& ServerSocket::instance(uint16_t port) {
    return *new ServerSocket(port);
}


ServerSocket& ServerSocket::instance(const std::string& hostname, uint16_t port) {
    return *new ServerSocket(hostname, port);
}


void ServerSocket::process(Request& request, Response& response) {
    // if GET-Request
    if (request.isGet()) {
        if (getProcessor) {
            getProcessor(request, response);
        }
    }
    
    // if POST-Request
    if (request.isPost()) {
        if (postProcessor) {
            postProcessor(request, response);
        }
    }
    
    // if PUT-Request
    if (request.isPut()) {
        if  (putProcessor) {
            putProcessor(request, response);
        }
    }
    
    // For all:
    if (allProcessor) {
        allProcessor(request, response);
    }
}


void ServerSocket::readEvent() {
    struct sockaddr_in remoteAddress;
    socklen_t addrlen = sizeof(remoteAddress);
    
    int csFd = ::accept(this->getFd(), (struct sockaddr*) &remoteAddress, &addrlen);
    
    if (csFd >= 0) {
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);
    
        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) == 0) {
            AcceptedSocket* cs = new AcceptedSocket(csFd, this, InetAddress(remoteAddress), InetAddress(localAddress));
            SocketMultiplexer::instance().getReadManager().manageSocket(cs);
        } else {
            shutdown(csFd, SHUT_RDWR);
            close(csFd);
        }
    }
}


void ServerSocket::serverRoot(std::string rootDir) {
    this->rootDir = rootDir;
}


std::string& ServerSocket::getRootDir() {
    return rootDir;
}


void ServerSocket::run() {
    SocketMultiplexer::instance().run();
}

