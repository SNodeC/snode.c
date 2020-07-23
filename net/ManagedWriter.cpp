#include "ManagedWriter.h"


int ManagedWriter::dispatch(fd_set& fdSet, int count) {
    for_each(descriptors.begin(), descriptors.end(), 
        [&fdSet, &count] (Writer* writer) -> void {
            if (FD_ISSET(dynamic_cast<Descriptor*>(writer)->getFd(), &fdSet)) {
                count--;
                writer->writeEvent();
            }
        }
    );
    
    return count;
}
