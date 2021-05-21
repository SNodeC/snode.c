#include "net/system/socket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    int socket(int domain, int type, int protocol) {
        errno = 0;
        return ::socket(domain, type, protocol);
    }

    int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
        errno = 0;
        return ::bind(sockfd, addr, addrlen);
    }

    int listen(int sockfd, int backlog) {
        errno = 0;
        return ::listen(sockfd, backlog);
    }

    int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
        errno = 0;
        return ::accept4(sockfd, addr, addrlen, flags);
    }
    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
        errno = 0;
        return ::connect(sockfd, addr, addrlen);
    }

    int getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
        errno = 0;
        return ::getsockname(sockfd, addr, addrlen);
    }

    int getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
        errno = 0;
        return ::getpeername(sockfd, addr, addrlen);
    }

    ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
        errno = 0;
        return ::recv(sockfd, buf, len, flags);
    }

    ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
        errno = 0;
        return ::send(sockfd, buf, len, flags);
    }

    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
        errno = 0;
        return ::getsockopt(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
        errno = 0;
        return ::setsockopt(sockfd, level, optname, optval, optlen);
    }

    int shutdown(int sockfd, int how) {
        errno = 0;
        return ::shutdown(sockfd, how);
    }

} // namespace net::system
