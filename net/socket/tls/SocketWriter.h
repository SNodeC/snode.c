#ifndef SSLSOCKETWRITER_H
#define SSLSOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketWriterBase.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketWriter
        : public SocketWriterBase
        , virtual public tls::Socket {
    public:
        using tls::Socket::send;
        virtual ssize_t send(const char* junk, const ssize_t& junkSize);

    protected:
        SocketWriter(const std::function<void(int errnum)>& onError)
            : SocketWriterBase(onError) {
        }
    };

}; // namespace tls

#endif // SSLSOCKETWRITER_H
