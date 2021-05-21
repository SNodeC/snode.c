#ifndef UNISTD_H
#define UNISTD_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <fcntl.h>
#include <stddef.h> // for size_t
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h> // for ssize_t

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    // #include <sys/types.h>, #include <sys/stat.h>, #include <fcntl.h>
    int open(const char* pathname, int flags);

    // #include <unistd.h>
    ssize_t read(int fd, void* buf, size_t count);
    ssize_t write(int fd, const void* buf, size_t count);
    int close(int fd);
    int pipe2(int pipefd[2], int flags);

} // namespace net::system

#endif // UNISTD_H
