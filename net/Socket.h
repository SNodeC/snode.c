#ifndef SOCKET_H
#define SOCKET_H

#include "Descriptor.h"
#include "InetAddress.h"

class Socket {
public:
    virtual ~Socket();
    
    int bind(InetAddress& localAddress);
    
    void setLocalAddress(const InetAddress& localAddress);
    
    InetAddress& getLocalAddress();

protected:
    Socket();
    
    Socket(int fd) : fd(fd) {}
    
    int getSFd() {
        return fd;
    }
    
    InetAddress localAddress;
    
protected:
    int fd;
};

#endif // SOCKET_H
