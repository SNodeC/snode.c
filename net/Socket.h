#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketBase.h"


class Socket : public SocketBase {
public:
    virtual void setFd(int fd);
    
    virtual ~Socket();
    
protected:
    Socket();
    Socket(int fd);
    
    ssize_t socketRecv(void *buf, size_t len, int flags);
    ssize_t socketSend(const void *buf, size_t len, int flags);
};

#endif // SOCKET_H
