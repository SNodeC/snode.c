#ifndef SOCKETEXCEPTIONMANAGER_H
#define SOCKETEXCEPTIONMANAGER_H

#include "Manager.h"
#include "Exception.h"

class ExceptionManager : public Manager<Exception> 
{
public:
    virtual int process(fd_set& fdSet, int count);
};

#endif // SOCKETEXCEPTIONMANAGER_H
