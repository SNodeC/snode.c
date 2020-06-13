#ifndef SOCKETREADMANAGER_H
#define SOCKETREADMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "Reader.h"


class ManagedReader : public Manager<Reader> {
public:
    int dispatch(fd_set& fdSet, int count) override;
};

#endif // SOCKETREADMANAGER_H
