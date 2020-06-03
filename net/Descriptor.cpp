#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"


Descriptor::Descriptor()
    : _fd(-1) {
}


Descriptor::~Descriptor() {
    ::close(_fd);
}


void Descriptor::attachFd(int fd) {
    this->_fd = fd;
}


int Descriptor::fd() const {
    if (_fd < 0) {
        std::cout << "Descriptor not initialized" << std::endl;
    }

    return _fd;
}
