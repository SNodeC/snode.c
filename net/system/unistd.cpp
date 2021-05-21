#include "net/system/unistd.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    int open(const char* pathname, int flags) {
        errno = 0;
        return ::open(pathname, flags);
    }

    ssize_t read(int fd, void* buf, size_t count) {
        errno = 0;
        return ::read(fd, buf, count);
    }

    ssize_t write(int fd, const void* buf, size_t count) {
        errno = 0;
        return ::write(fd, buf, count);
    }

    int close(int fd) {
        errno = 0;
        return ::close(fd);
    }

    int pipe2(int pipefd[2], int flags) {
        errno = 0;
        return ::pipe2(pipefd, flags);
    }

} // namespace net::system
