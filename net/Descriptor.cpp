#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"


Descriptor::~Descriptor() {
    ::close(fd);
}


void Descriptor::attachFd(int fd) {
    this->fd = fd;
}


void Descriptor::setNonBlocking() {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    }
}


int Descriptor::getFd() const {
    if (fd < 0) {
        std::cout << "Descriptor not initialized" << std::endl;
    }

    return fd;
}
