#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/Socket.h"


namespace legacy {

    class Socket : public ::Socket {
    public:
        virtual ~Socket();

    protected:
        Socket() = default;
        Socket(int fd);

        ssize_t recv(void* buf, size_t len, int flags);
        ssize_t send(const void* buf, size_t len, int flags);
    };

}; // namespace legacy

#endif // SOCKET_H
