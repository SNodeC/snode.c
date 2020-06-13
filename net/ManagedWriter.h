#ifndef SOCKETWRITEMANAGER_H
#define SOCKETWRITEMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "Writer.h"


class ManagedWriter : public Manager<Writer> {
public:
    int dispatch(fd_set& fdSet, int count) override;
};

#endif // SOCKETWRITEMANAGER_H
