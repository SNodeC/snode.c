#ifndef TLS_SOCKETCLIENT_H
#define TLS_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketClient.h"
#include "socket/tls/SocketConnection.h"

namespace tls {

    class SocketClient : public ::SocketClient<tls::SocketConnection> {
    public:
        using ::SocketClient<tls::SocketConnection>::SocketClient;
    };

} // namespace tls

#endif // TLS_SOCKETCLIENT_H
