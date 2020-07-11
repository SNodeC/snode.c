#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedServer.h"


int ManagedServer::dispatch(const fd_set& fdSet, int count) {
    for (Server* server : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(server)->getFd(), &fdSet)) {
            count--;
            server->acceptEvent();
        }
    }

    return count;
}
