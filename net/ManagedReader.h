#ifndef MANAGEDREADER_H
#define MANAGEDREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "ReaderEvent.h"

// IWYU pragma: no_forward_declare Reader

class ManagedReader : public Manager<ReadEvent> {
public:
    using Manager<ReadEvent>::Manager;
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDREADER_H
