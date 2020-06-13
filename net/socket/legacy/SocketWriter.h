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
    protected:
        explicit SocketWriter(const std::function<void(int errnum)>& onError)
            : SocketWriterBase(onError) {
        }

        ssize_t send(const char* junk, const ssize_t& junkSize) override;
    };

}; // namespace legacy

#endif // SOCKETWRITER_H
