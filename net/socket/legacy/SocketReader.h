#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "socket/legacy/Socket.h"


class SocketConnection;

namespace legacy {

    class SocketReader
        : public Reader
        , virtual public legacy::Socket {
    public:
        void readEvent();

    protected:
        SocketReader()
            : readProcessor(0) {
        }

        SocketReader(const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(int errnum)>& onError)
            : legacy::Socket()
            , Reader(onError)
            , readProcessor(readProcessor) {
        }

        std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;
    };

}; // namespace legacy

#endif // SOCKETREADER_H
