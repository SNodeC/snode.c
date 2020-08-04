#ifndef MANAGEDEXCEPTIONS_H
#define MANAGEDEXCEPTIONS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Exception.h"
#include "Manager.h"

// IWYU pragma: no_forward_declare Exception

class ManagedExceptions : public Manager<Exception> {
public:
    using Manager<Exception>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDEXCEPTIONS_H
