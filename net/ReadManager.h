#ifndef SOCKETREADMANAGER_H
#define SOCKETREADMANAGER_H

#include "SocketManager.h"
#include "Reader.h"


class ReadManager : public SocketManager<Reader>
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETREADMANAGER_H
