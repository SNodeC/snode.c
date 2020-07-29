#ifndef MANAGEDREADER_H
#define MANAGEDREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "Reader.h"

class ManagedReader : public Manager<Reader> {
public:
    using Manager<Reader>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDREADER_H
