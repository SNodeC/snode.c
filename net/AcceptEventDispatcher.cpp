#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AcceptEventDispatcher.h"
#include "Descriptor.h" // for Descriptor

// IWYU pragma: no_include "Server.h"

int AcceptEventDispatcher::dispatch(const fd_set& fdSet, int count) {
    if (count > 0) {
        for (auto [fd, eventReceiver] : observedEvents) {
            if (count == 0) {
                break;
            }
            if (FD_ISSET(fd, &fdSet)) {
                count--;
                eventReceiver->acceptEvent();
            }
        }
    }

    return count;
}
