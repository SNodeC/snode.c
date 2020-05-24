#ifndef SOCKETREADMANAGER_H
#define SOCKETREADMANAGER_H

#include "Manager.h"
#include "Reader.h"


class ManagedReader : public Manager<Reader>
{
public:
    virtual int dispatch(fd_set& fdSet, int count);
};

#endif // SOCKETREADMANAGER_H
