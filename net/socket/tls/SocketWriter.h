#ifndef SSLSOCKETWRITER_H
#define SSLSOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"
#include "socket/tls/Socket.h"


namespace tls {

    class SocketWriter
        : public Writer
        , virtual public tls::Socket {
    public:
        void writeEvent();

    protected:
        SocketWriter(const std::function<void(int errnum)>& onError)
            : Writer(onError) {
        }
    };

}; // namespace tls

#endif // SSLSOCKETWRITER_H
