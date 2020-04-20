#include "Descriptor.h"

#include <unistd.h>

Descriptor::Descriptor(int fd) : fd(fd) {
}

Descriptor::~Descriptor() {
    ::close(fd);
}

int Descriptor::getFd() const {
    return fd;
}
