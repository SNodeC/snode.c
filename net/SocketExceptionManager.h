#ifndef SOCKETEXCEPTIONMANAGER_H
#define SOCKETEXCEPTIONMANAGER_H

#include "SocketManager.h"

class SocketExceptionManager : public SocketManager
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETEXCEPTIONMANAGER_H
