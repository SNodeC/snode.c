#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedServer.h"

// IWYU pragma: no_include "Server.h"

int ManagedServer::dispatch(const fd_set& fdSet, int count) {
    for (std::pair<int, Server*> serverPair : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(serverPair.second)->getFd(), &fdSet)) {
            count--;
            serverPair.second->acceptEvent();
        }
    }

    return count;
}
