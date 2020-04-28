#include <unistd.h>       

#include "ServerSocket.h"
#include "ConnectedSocket.h"
#include "SocketMultiplexer.h"


ServerSocket::ServerSocket(std::function<void (ConnectedSocket* cs)> onConnect,
                           std::function<void (ConnectedSocket* cs)> onDisconnect,
                           std::function<void (ConnectedSocket* cs, std::string line)> readProcessor) 
: SocketReader(), onConnect(onConnect), onDisconnect(onDisconnect), readProcessor(readProcessor) {
    this->open();
    SocketMultiplexer::instance().getReadManager().manageSocket(this);
}


ServerSocket::ServerSocket(uint16_t port, 
                           std::function<void (ConnectedSocket* cs)> onConnect,
                           std::function<void (ConnectedSocket* cs)> onDisconnect,
                           std::function<void (ConnectedSocket* cs, std::string line)> readProcessor) 
: ServerSocket(onConnect, onDisconnect, readProcessor) {
    int sockopt = 1;
    setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    
    localAddress = InetAddress(port);
    this->bind(localAddress);
    this->listen(5);
}


ServerSocket& ServerSocket::instance(uint16_t port,
                                     std::function<void (ConnectedSocket* cs)> onConnect,
                                     std::function<void (ConnectedSocket* cs)> onDisconnect,
                                     std::function<void (ConnectedSocket* cs, std::string line)> readProcessor) {
    return *new ServerSocket(port, onConnect, onDisconnect, readProcessor);
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


void ServerSocket::run() {
    SocketMultiplexer::run();
}

