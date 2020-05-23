#ifndef SOCKETBASE_H
#define SOCKETBASE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "Descriptor.h"


class SocketBase : virtual public Descriptor {
public:
    virtual void setFd(int fd);
    
    virtual ~SocketBase();
    
    void bind(InetAddress& localAddress, const std::function<void (int errnum)>& onError);
    
    void listen(int backlog, const std::function<void (int errnum)>& onError);
    
    InetAddress& getLocalAddress();
    
    void setLocalAddress(const InetAddress& localAddress);
    
protected:
    SocketBase();
//    SocketBase(int fd);
    
    void open(const std::function<void (int errnum)>& onError);
    
    virtual ssize_t socketRecv(void *buf, size_t len, int flags) = 0;
    virtual ssize_t socketSend(const void *buf, size_t len, int flags) = 0;
    
    
    InetAddress localAddress;
};

#endif // SOCKETBASE_H
