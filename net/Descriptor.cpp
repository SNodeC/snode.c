#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"


Descriptor::Descriptor() : fd(-1) {}


Descriptor::Descriptor(int fd) {
    this->fd = fd;
}


Descriptor::~Descriptor() {
    ::close(fd);
}


int Descriptor::getFd() const {
    if (fd < 0) {
        std::cout << "Descriptor not initialized" << std::endl;
    }

    return fd;
}
