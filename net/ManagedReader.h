#ifndef MANAGEDREADER_H
#define MANAGEDREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Manager.h"
#include "Reader.h"


class ManagedReader : public Manager<Reader> {
public:
    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDREADER_H
