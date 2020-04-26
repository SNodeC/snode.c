#include <unistd.h>

#include "Descriptor.h"


Descriptor::Descriptor() {}


Descriptor::~Descriptor() {
    ::close(fd);
}

int Descriptor::getFd() const {
    return fd;
}
