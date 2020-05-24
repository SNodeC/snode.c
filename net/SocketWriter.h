#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"
#include "Socket.h"


class SocketWriter : public Writer, virtual public Socket {
public:
    void writeEvent();

protected:
    SocketWriter(const std::function<void (int errnum)>& onError) : Writer(onError) {}
};

#endif // SOCKETWRITER_H
