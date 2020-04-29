#ifndef SOCKETREADMANAGER_H
#define SOCKETREADMANAGER_H

#include "Manager.h"
#include "Reader.h"


class ReadManager : public Manager<Reader>
{
public:
    int process(fd_set& fdSet, int count);
};

#endif // SOCKETREADMANAGER_H
