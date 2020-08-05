#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "OutOfBandEventDispatcher.h"

// IWYU pragma: no_include "Exception.h"

int OutOfBandEventDispatcher::dispatch(const fd_set& fdSet, int count) {
    if (count > 0) {
        for (auto [fd, eventReceiver] : observedEvents) {
            if (count == 0) {
                break;
            }
            if (FD_ISSET(fd, &fdSet)) {
                count--;
                eventReceiver->outOfBandEvent();
            }
        }
    }

    return count;
}
