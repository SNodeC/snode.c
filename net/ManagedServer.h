#ifndef MANAGEDSERVER_H
#define MANAGEDSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AcceptEvent.h"
#include "Manager.h"

// IWYU pragma: no_forward_declare Server

class ManagedServer : public Manager<AcceptEvent> {
public:
    using Manager<AcceptEvent>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDSERVER_H
