#ifndef MANAGEDWRITER_H
#define MANAGEDWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h> // for fd_set

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "Writer.h"


class ManagedWriter : public Manager<Writer> {
public:
    using Manager<Writer>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDWRITER_H
