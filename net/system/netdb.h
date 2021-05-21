#ifndef NETDB_H
#define NETDB_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    // #include <sys/types.h>, #include <sys/socket.h>, #include <netdb.h>
    int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);

    // #include <sys/types.h>, #include <sys/socket.h>, #include <netdb.h>
    void freeaddrinfo(struct addrinfo* res);

    // #include <sys/socket>, #include <netdb.h>
    int
    getnameinfo(const struct sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags);

} // namespace net::system

#endif // NETDB_H
