#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "Descriptor.h"


class Socket : virtual public Descriptor {
public:
    void setFd(int fd);
    
    virtual ~Socket();
    
    void bind(InetAddress& localAddress, const std::function<void (int errnum)>& onError);
    
    InetAddress& getLocalAddress();
    
    void setLocalAddress(const InetAddress& localAddress);
    
protected:
    Socket();
    Socket(int fd);
    
    void open(const std::function<void (int errnum)>& onError);
    
    ssize_t recv(void *buf, size_t len, int flags);
    ssize_t send(const void *buf, size_t len, int flags);
    

    InetAddress localAddress;
};

#endif // SOCKET_H
