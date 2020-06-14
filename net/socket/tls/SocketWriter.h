#ifndef TLS_SOCKETWRITER_H
#define TLS_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketWriterBase.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketWriter
        : public SocketWriterBase
        , virtual public tls::Socket {
    protected:
        explicit SocketWriter(const std::function<void(int errnum)>& onError)
            : SocketWriterBase(onError) {
        }

        ssize_t send(const char* junk, const ssize_t& junkSize) override;
    };

}; // namespace tls

#endif // TLS_SOCKETWRITER_H
