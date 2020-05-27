#ifndef SOCKETEXCEPTIONMANAGER_H
#define SOCKETEXCEPTIONMANAGER_H

#include "Exception.h"
#include "Manager.h"

class ManagedExceptions : public Manager<Exception> {
public:
    virtual int dispatch(fd_set& fdSet, int count);
};

#endif // SOCKETEXCEPTIONMANAGER_H
