#ifndef SOCKETEXCEPTIONMANAGER_H
#define SOCKETEXCEPTIONMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Exception.h"
#include "Manager.h"


class ManagedExceptions : public Manager<Exception> {
public:
    virtual int dispatch(fd_set& fdSet, int count);
};

#endif // SOCKETEXCEPTIONMANAGER_H
