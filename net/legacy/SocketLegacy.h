#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Socket.h"


namespace legacy {

class SocketLegacy : public Socket {
public:
    virtual ~SocketLegacy();

protected:
    SocketLegacy() = default;
    SocketLegacy(int fd);

    ssize_t socketRecv(void* buf, size_t len, int flags);
    ssize_t socketSend(const void* buf, size_t len, int flags);
};

};

#endif // SOCKET_H
