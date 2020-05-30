#include "ManagedWriter.h"


int ManagedWriter::dispatch(fd_set& fdSet, int count) {
    for (Writer* writer : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(writer)->getFd(), &fdSet)) {
            count--;
            writer->writeEvent();
        }
    }

    return count;
}
