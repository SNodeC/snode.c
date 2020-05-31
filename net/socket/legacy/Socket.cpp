#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/Socket.h"


namespace legacy {

    Socket::Socket(int fd)
        : ::Socket() {
        this->attach(fd);
    }


    Socket::~Socket() {
    }


    ssize_t Socket::socketRecv(void* buf, size_t len, int flags) {
        return ::recv(this->getFd(), buf, len, flags);
    }


    ssize_t Socket::socketSend(const void* buf, size_t len, int flags) {
        return ::send(this->getFd(), buf, len, flags);
    }

}; // namespace legacy
