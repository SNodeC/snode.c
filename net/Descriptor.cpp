#include "Descriptor.h"

#include <iostream>

#include <unistd.h>

Descriptor::Descriptor() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}
/*
Descriptor::Descriptor(int fd) : fd(fd) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}
*/

Descriptor::~Descriptor() {
    ::close(fd);
}

int Descriptor::getFd() const {
    return fd;
}
