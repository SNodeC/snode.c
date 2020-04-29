#include "ClientSocket.h"

#include "Multiplexer.h"


/*
ClientSocket::ClientSocket(int csfd) : ConnectedSocket(csfd), request(this) {
}


ClientSocket* ClientSocket::connect(const InetAddress& ina, std::function<void (Request& req)> callback) {
    int csFd = ::socket(AF_INET, SOCK_STREAM, 0);
    
    int ret = ::connect(csFd, (struct sockaddr*) &(ina.getSockAddr()), sizeof(struct sockaddr_in));
    
    ClientSocket* cs = 0;
    
    if (ret == 0) {
        cs = new ClientSocket(csFd);
        
        cs->setRemoteAddress(ina);
        
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);
        
        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) < 0) {
            delete cs;
            return 0;
        }
        
        cs->setLocalAddress(InetAddress(localAddress));
        
        cs->callback = callback;
    
        SocketMultiplexer::instance().getReadManager().manageSocket(cs);
    }
    
    return cs;
}


void ClientSocket::push(const char* junk, int n) {
}


void ClientSocket::ready() {
    callback(request);
}
*/
