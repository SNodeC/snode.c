#include "net/system/netdb.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res) {
        errno = 0;
        return ::getaddrinfo(node, service, hints, res);
    }

    void freeaddrinfo(struct addrinfo* res) {
        errno = 0;
        return ::freeaddrinfo(res);
    }

    int
    getnameinfo(const struct sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags) {
        errno = 0;
        return ::getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
    }

} // namespace net::system
