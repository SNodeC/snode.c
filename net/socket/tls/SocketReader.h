#ifndef SSLSOCKETREADER_H
#define SSLSOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "socket/tls/Socket.h"


class SocketConnection;

namespace tls {

    class SocketReader
        : public Reader
        , virtual public tls::Socket {
    public:
        void readEvent();

    protected:
        SocketReader()
            : readProcessor(0) {
        }

        SocketReader(const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(int errnum)>& onError)
            : tls::Socket()
            , Reader(onError)
            , readProcessor(readProcessor) {
        }

        std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;
    };

}; // namespace tls

#endif // SSLSOCKETREADER_H
