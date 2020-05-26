#include "Descriptor.h"

#include <unistd.h>

Descriptor::Descriptor(int fd) : fd(fd), managedCount(0) {
}

Descriptor::~Descriptor() {
    ::close(fd);
}

int Descriptor::getFd() const {
    return fd;
}


void Descriptor::incManagedCount() {
    managedCount++;
}

void Descriptor::decManagedCount() {
    managedCount--;
    
    if (managedCount == 0) {
        delete this;
    }
}
