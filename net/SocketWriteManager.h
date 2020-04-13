#ifndef SOCKETWRITEMANAGER_H
#define SOCKETWRITEMANAGER_H

#include "SocketManager.h"

/**
 * @todo write docs
 */
class SocketWriteManager : public SocketManager
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETWRITEMANAGER_H
