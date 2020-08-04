#ifndef TLS_SOCKETCLIENT_H
#define TLS_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketClient.h"
#include "socket/tls/SocketConnection.h"

namespace tls {

    class SocketClient : public ::SocketClient<tls::SocketConnection> {
    public:
        SocketClient(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                     const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError);

        ~SocketClient() override;

        // NOLINTNEXTLINE(google-default-arguments)
        void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                     const InetAddress& localAddress = InetAddress()) override;

    private:
        SSL_CTX* ctx;

        std::function<void(tls::SocketConnection* cs)> onConnect;
        std::function<void(tls::SocketConnection* cs)> onDisconnect;
        std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)> onRead;
        std::function<void(tls::SocketConnection* cs, int errnum)> onReadError;
        std::function<void(tls::SocketConnection* cs, int errnum)> onWriteError;

        std::function<void(int err)> onError;
    };

} // namespace tls

#endif // TLS_SOCKETCLIENT_H
