#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <stddef.h> // for size_t
#include <sys/socket.h>
#include <sys/types.h> // for ssize_t

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    // #include <sys/socket.h>
    int getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    int getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    int shutdown(int sockfd, int how);

    // #include <sys/types.h>, #include <sys/socket.h>
    int socket(int domain, int type, int protocol);
    int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    int listen(int sockfd, int backlog);
    int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags);
    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    ssize_t recv(int sockfd, void* buf, size_t len, int flags);
    ssize_t send(int sockfd, const void* buf, size_t len, int flags);
    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

} // namespace net::system

#endif // SOCKET_H
