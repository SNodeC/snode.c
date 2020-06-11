#ifndef SSLSOCKETREADER_H
#define SSLSOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketReaderBase.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketReader
        : public SocketReaderBase
        , virtual public tls::Socket {
    public:
        using tls::Socket::recv;
        virtual ssize_t recv(char* junk, const ssize_t& junkSize);

    protected:
        SocketReader() {
        }

        SocketReader(const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(int errnum)>& onError)
            : tls::Socket()
            , SocketReaderBase(readProcessor, onError) {
        }
    };

}; // namespace tls

#endif // SSLSOCKETREADER_H
