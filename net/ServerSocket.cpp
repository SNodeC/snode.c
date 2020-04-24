#include "ServerSocket.h"
#include "AcceptedSocket.h"
#include "SocketMultiplexer.h"

Server::Server() : Socket(socket(AF_INET, SOCK_STREAM, 0)), rootDir(".") {
    SocketMultiplexer::instance().getReadManager().manageSocket(this);
}

/*
Server::Server(const Server& server) {
}
*/

Server::Server(uint16_t port) : Server() {
    int sockopt = 1;
    setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    
    localAddress = InetAddress(port);
    this->bind(localAddress);
    this->listen(5);
}


Server::Server(const std::string hostname, uint16_t port) : Server() {
}


Server& Server::instance(uint16_t port) {
    return *new Server(port);
}


Server& Server::instance(const std::string& hostname, uint16_t port) {
    return *new Server(hostname, port);
}


AcceptedSocket* Server::accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    
    int csFd = ::accept(this->getFd(), (struct sockaddr*) &addr, &addrlen);
    
    if (csFd < 0) {
        return 0;
    }
    
    AcceptedSocket* cs = new AcceptedSocket(csFd, this);
    
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


void Server::process(Request& request, Response& response) {
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


void Server::readEvent() {
    AcceptedSocket* as = this->accept();
    
    if (as) {
        SocketMultiplexer::instance().getReadManager().manageSocket(as);
    }
}


void Server::serverRoot(std::string rootDir) {
    this->rootDir = rootDir;
}


std::string& Server::getRootDir() {
    return rootDir;
}


void Server::run() {
    SocketMultiplexer::instance().run();
}
