#ifndef SOCKETWRITEMANAGER_H
#define SOCKETWRITEMANAGER_H

#include "Manager.h"
#include "Writer.h"


class WriteManager : public Manager<Writer>
{
public:
    virtual int process(fd_set& fdSet, int count);
};

#endif // SOCKETWRITEMANAGER_H
