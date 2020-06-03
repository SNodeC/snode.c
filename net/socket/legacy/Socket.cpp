#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/Socket.h"


namespace legacy {

    ssize_t Socket::recv(void* buf, size_t len, int flags) {
        return ::recv(this->fd(), buf, len, flags);
    }


    ssize_t Socket::send(const void* buf, size_t len, int flags) {
        return ::send(this->fd(), buf, len, flags);
    }

}; // namespace legacy
