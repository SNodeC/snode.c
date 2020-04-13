#ifndef SOCKETREADMANAGER_H
#define SOCKETREADMANAGER_H

#include "SocketManager.h"


class SocketReadManager : public SocketManager
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETREADMANAGER_H
