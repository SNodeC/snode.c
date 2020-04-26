#ifndef SOCKET_H
#define SOCKET_H

#include "InetAddress.h"


class Socket {
public:
    virtual ~Socket();
    
    int bind(InetAddress& localAddress);
    
    InetAddress& getLocalAddress();
    
    
protected:
    void setLocalAddress(const InetAddress& localAddress);
    Socket();
    
    int getSFd() {
        return fd;
    }
    
    void setSFd(int fd) {
        this->fd = fd;
    }
    
    InetAddress localAddress;
    
    int fd;
};

#endif // SOCKET_H
