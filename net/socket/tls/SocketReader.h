#ifndef TLS_SOCKETREADER_H
#define TLS_SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <stddef.h>    // for size_t
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketReader.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketReader
        : public ::SocketReader
        , virtual public tls::Socket {
    protected:
        SocketReader(const std::function<void(const char* junk, size_t n)>& onRead, const std::function<void(int errnum)>& onError)
            : ::SocketReader(onRead, onError) {
        }

    private:
        ssize_t recv(char* junk, size_t junkSize) override;
    };

}; // namespace tls

#endif // TLS_SOCKETREADER_H
