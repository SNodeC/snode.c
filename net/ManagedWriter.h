#ifndef MANAGEDWRITER_H
#define MANAGEDWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h> // for fd_set

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "WriteEvent.h"

// IWYU pragma: no_forward_declare Writer

class ManagedWriter : public Manager<WriteEvent> {
public:
    using Manager<WriteEvent>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDWRITER_H
