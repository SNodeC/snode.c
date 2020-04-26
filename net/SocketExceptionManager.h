#ifndef SOCKETEXCEPTIONMANAGER_H
#define SOCKETEXCEPTIONMANAGER_H

#include "SocketManager.h"
#include "Exception.h"

class SocketExceptionManager : public SocketManager<Exception> 
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETEXCEPTIONMANAGER_H
