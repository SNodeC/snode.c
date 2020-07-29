#ifndef MANAGEDSERVER_H
#define MANAGEDSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "Server.h"

class ManagedServer : public Manager<Server> {
public:
    using Manager<Server>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDSERVER_H
