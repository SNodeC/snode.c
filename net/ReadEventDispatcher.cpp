#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map> // for map
#include <tuple>
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ReadEventDispatcher.h"

// IWYU pragma: no_include "Reader.h"

int ReadEventDispatcher::dispatch(const fd_set& fdSet, int count) {
    if (count > 0) {
        for (auto [fd, eventReceiver] : observedEvents) {
            if (count == 0) {
                break;
            }
            if (FD_ISSET(fd, &fdSet)) {
                count--;
                eventReceiver->readEvent();
            }
        }
    }

    return count;
}
