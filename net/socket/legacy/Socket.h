#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/Socket.h"


namespace legacy {

    class Socket : public ::Socket {
    protected:
        Socket() = default;
    };

}; // namespace legacy

#endif // SOCKET_H
