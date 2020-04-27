#include <unistd.h>       

#include "ServerSocket.h"
#include "AcceptedSocket.h"
#include "SocketMultiplexer.h"


ServerSocket::ServerSocket(std::function<void (ConnectedSocket* cs)> onConnect,
                           std::function<void (ConnectedSocket* cs)> onDisconnect,
                           std::function<void (ConnectedSocket* cs, std::string line)> rp) 
    : SocketReader(), onConnect(onConnect), onDisconnect(onDisconnect), readProcessor(rp), rootDir(".") {
    SocketMultiplexer::instance().getReadManager().manageSocket(this);
}


ServerSocket::ServerSocket(uint16_t port, 
                           std::function<void (ConnectedSocket* cs)> oc,
                           std::function<void (ConnectedSocket* cs)> od,
                           std::function<void (ConnectedSocket* cs, std::string line)> rp) 
    : ServerSocket(oc, od, rp) {
    int sockopt = 1;
    setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    
    localAddress = InetAddress(port);
    this->bind(localAddress);
    this->listen(5);
}


ServerSocket& ServerSocket::instance(uint16_t port,
                                     std::function<void (ConnectedSocket* cs)> oc,
                                     std::function<void (ConnectedSocket* cs)> od,
                                     std::function<void (ConnectedSocket* cs, std::string line)> rp) {
    return *new ServerSocket(port, oc, od, rp);
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


void ServerSocket::readEvent1() {
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


void ServerSocket::readEvent() {
    struct sockaddr_in remoteAddress;
    socklen_t addrlen = sizeof(remoteAddress);
    
    int csFd = ::accept(this->getFd(), (struct sockaddr*) &remoteAddress, &addrlen);
    
    if (csFd >= 0) {
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);
        
        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) == 0) {
            ConnectedSocket* cs = new ConnectedSocket(csFd, this, this->readProcessor);
            cs->setRemoteAddress(remoteAddress);
            cs->setLocalAddress(localAddress);
            SocketMultiplexer::instance().getReadManager().manageSocket(cs);
            onConnect(cs);
        } else {
            shutdown(csFd, SHUT_RDWR);
            close(csFd);
        }
    }
}
    

void ServerSocket::disconnect(ConnectedSocket* cs) {
    onDisconnect(cs);
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

