#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedServer.h"

// IWYU pragma: no_include "Server.h"

int ManagedServer::dispatch(const fd_set& fdSet, int count) {
    for (auto [fd, server] : descriptors) {
        if (FD_ISSET(fd, &fdSet)) {
            count--;
            server->acceptEvent();
        }
    }

    return count;
}
