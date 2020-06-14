#ifndef TLS_SOCKETREADER_H
#define TLS_SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketReaderBase.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketReader
        : public SocketReaderBase
        , virtual public tls::Socket {
    protected:
        SocketReader() = default;

        SocketReader(const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(int errnum)>& onError)
            : SocketReaderBase(readProcessor, onError) {
        }

        ssize_t recv(char* junk, const ssize_t& junkSize) override;
    };

}; // namespace tls

#endif // TLS_SOCKETREADER_H
