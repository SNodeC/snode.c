#ifndef SOCKETSERVERINTERFACE_H
#define SOCKETSERVERINTERFACE_H

#include <netinet/in.h>

#include <functional>

class SocketConnectionInterface;

class SocketServerInterface
{
public:
    virtual void listen(in_port_t port, int backlog, const std::function<void (int err)>& onError) = 0;
    
    virtual void readEvent() = 0;
    
    virtual void disconnect(SocketConnectionInterface* cs) = 0;
};

#endif // SOCKETSERVERINTERFACE_H
