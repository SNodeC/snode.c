#include <unistd.h>       

#include "ServerSocket.h"
#include "ConnectedSocket.h"
#include "Multiplexer.h"


ServerSocket::ServerSocket(const std::function<void (ConnectedSocket* cs)>& onConnect,
                           const std::function<void (ConnectedSocket* cs)>& onDisconnect,
                           const std::function<void (ConnectedSocket* cs, const char*  junk, ssize_t n)>& readProcessor) 
: SocketReader(), onConnect(onConnect), onDisconnect(onDisconnect), readProcessor(readProcessor) 
{}


ServerSocket& ServerSocket::instance(const std::function<void (ConnectedSocket* cs)>& onConnect,
                                     const std::function<void (ConnectedSocket* cs)>& onDisconnect,
                                     const std::function<void (ConnectedSocket* cs, const char*  junk, ssize_t n)>& readProcessor) 
{
    return *new ServerSocket(onConnect, onDisconnect, readProcessor);
}


void ServerSocket::listen(in_port_t port, int backlog, const std::function<void (int err)>& callback) {
    this->callback = callback;
    if (this->open() < 0) {
        if (callback) callback(errno);
    } else {
        int sockopt = 1;
    
        if (setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
            if (callback) callback(errno);
        } else {
            localAddress = InetAddress(port);
            this->bind(localAddress);
    
            if (::listen(this->getFd(), backlog) < 0) {
                if (callback) callback(errno);
            } else {
                Multiplexer::instance().getReadManager().manageSocket(this);
                if (callback) callback(0);
            }
        }
    }
}


void ServerSocket::readEvent() {
    struct sockaddr_in remoteAddress;
    socklen_t addrlen = sizeof(remoteAddress);
    
    int csFd = -1;
    
    do {
        errno = 0;
        csFd = ::accept(this->getFd(), (struct sockaddr*) &remoteAddress, &addrlen);
    } while (csFd < 0 && errno == EINTR);
    
    if (csFd >= 0) {
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);
        
        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) == 0) {
            ConnectedSocket* cs = new ConnectedSocket(csFd, this, this->readProcessor);
            
            cs->setRemoteAddress(remoteAddress);
            cs->setLocalAddress(localAddress);
            
            Multiplexer::instance().getReadManager().manageSocket(cs);
            onConnect(cs);
        } else {
            shutdown(csFd, SHUT_RDWR);
            close(csFd);
            callback(errno);
        }
    } else {
        callback(errno);
    }
}
    

void ServerSocket::disconnect(ConnectedSocket* cs) {
    onDisconnect(cs);
}


void ServerSocket::run() {
    Multiplexer::run();
}

