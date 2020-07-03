#ifndef TLS_SOCKETREADER_H
#define TLS_SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketReader.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketReader
        : public ::SocketReader
        , virtual public tls::Socket {
    protected:
        SocketReader(const std::function<void(const char* junk, ssize_t n)>& readProcessor, const std::function<void(int errnum)>& onError)
            : ::SocketReader(readProcessor, onError) {
        }

    private:
        ssize_t recv(char* junk, const ssize_t& junkSize) override;
    };

}; // namespace tls

#endif // TLS_SOCKETREADER_H
