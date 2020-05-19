#ifndef SOCKETWRITEMANAGER_H
#define SOCKETWRITEMANAGER_H

#include "SocketManager.h"
#include "Writer.h"

class SocketWriteManager : public SocketManager<Writer>
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETWRITEMANAGER_H
