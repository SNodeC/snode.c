#ifndef TLS_SOCKETWRITER_H
#define TLS_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <stddef.h>    // for size_t
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketWriter.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketWriter : public ::SocketWriter<tls::Socket> {
    protected:
        using ::SocketWriter<tls::Socket>::SocketWriter;

        ssize_t send(const char* junk, const size_t junkSize) override;
    };

}; // namespace tls

#endif // TLS_SOCKETWRITER_H
