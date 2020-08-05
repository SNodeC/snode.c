#ifndef MANAGEDWRITER_H
#define MANAGEDWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h> // for fd_set

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventDispatcher.h"
#include "WriteEventReceiver.h"

// IWYU pragma: no_forward_declare Writer

class WriteEventDispatcher : public EventDispatcher<WriteEventReceiver> {
public:
    using EventDispatcher<WriteEventReceiver>::EventDispatcher;

    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDWRITER_H
