#ifndef SOCKET_H
#define SOCKET_H

#include <functional>

#include "InetAddress.h"
#include "Descriptor.h"


class Socket : virtual public Descriptor {
public:
    virtual ~Socket();
    
    void bind(InetAddress& localAddress, const std::function<void (int errnum)>& onError);
    
    InetAddress& getLocalAddress();
    
    void setLocalAddress(const InetAddress& localAddress);
    
protected:
    Socket();
    void open(const std::function<void (int errnum)>& onError);

    InetAddress localAddress;
};

#endif // SOCKET_H
