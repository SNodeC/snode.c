#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#include <sys/select.h>
#include <list>

class Socket;

class SocketManager
{
public:
    SocketManager();
    
    virtual ~SocketManager();
    
    fd_set& getFdSet();
    
    void manageSocket(Socket* socket);
    
    void unmanageSocket(Socket* socket);
    
    int getMaxFd();
    
protected:
    int updateMaxFd();
    
    fd_set fdSet;
    int maxFd;
    std::list<Socket*> sockets;
    std::list<Socket*> addedSockets;
    std::list<Socket*> removedSockets;
};

#endif // SOCKETMANAGER_H
