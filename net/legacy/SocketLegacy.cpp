#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketLegacy.h"


namespace legacy {

SocketLegacy::SocketLegacy(int fd)
    : Socket() {
    this->attach(fd);
}


SocketLegacy::~SocketLegacy() {
}


ssize_t SocketLegacy::socketRecv(void* buf, size_t len, int flags) {
    return ::recv(this->getFd(), buf, len, flags);
}


ssize_t SocketLegacy::socketSend(const void* buf, size_t len, int flags) {
    return ::send(this->getFd(), buf, len, flags);
}

};
