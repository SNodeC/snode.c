#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"
#include "socket/legacy/Socket.h"


namespace legacy {

    class SocketWriter
        : public Writer
        , virtual public legacy::Socket {
    public:
        void writeEvent();

    protected:
        SocketWriter(const std::function<void(int errnum)>& onError)
            : Writer(onError) {
        }
    };

}; // namespace legacy

#endif // SOCKETWRITER_H
