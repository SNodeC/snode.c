#ifndef SOCKET_H
#define SOCKET_H

#include "InetAddress.h"

class Socket {
public:
    Socket();
    
    virtual ~Socket();
    
    int getFd() const;
    
    void setFd(int fd);
    
    int bind(InetAddress& localAddress);
    
    void setLocalAddress(const InetAddress& localAddress);
    
    InetAddress& getLocalAddress();
    
    void incManagedCount();
    
    void decManagedCount();
    
protected:
    InetAddress localAddress;
    int fd;
    int managedCount;
};

#endif // SOCKET_H
