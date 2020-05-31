#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketWriterBase.h"
#include "socket/legacy/Socket.h"


namespace legacy {

    class SocketWriter
        : public SocketWriterBase
        , virtual public legacy::Socket {
    public:
        using legacy::Socket::send;
        virtual ssize_t send(const char* junk, const ssize_t& junkSize);

    protected:
        SocketWriter(const std::function<void(int errnum)>& onError)
            : SocketWriterBase(onError) {
        }
    };

}; // namespace legacy

#endif // SOCKETWRITER_H
