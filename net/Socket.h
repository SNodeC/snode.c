#ifndef SOCKET_H
#define SOCKET_H

#include "InetAddress.h"
#include "Descriptor.h"


class Socket : virtual public Descriptor {
public:
    virtual ~Socket();
    
    int bind(InetAddress& localAddress);
    
    InetAddress& getLocalAddress();
    
    void setLocalAddress(const InetAddress& localAddress);
    
protected:
    Socket();
    void open();

    InetAddress localAddress;
};

#endif // SOCKET_H
