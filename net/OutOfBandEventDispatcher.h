#ifndef MANAGEDEXCEPTIONS_H
#define MANAGEDEXCEPTIONS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventDispatcher.h"
#include "OutOfBandEventReceiver.h"

// IWYU pragma: no_forward_declare Exception

class OutOfBandEventDispatcher : public EventDispatcher<OutOfBandEventReceiver> {
public:
    using EventDispatcher<OutOfBandEventReceiver>::EventDispatcher;

    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDEXCEPTIONS_H
