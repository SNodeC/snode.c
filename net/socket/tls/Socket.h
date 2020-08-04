#ifndef TLS_SOCKET_H
#define TLS_SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // IWYU pragma: keep // for SSL, SSL_CTX

// IWYU pragma: no_include <openssl/ossl_typ.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/Socket.h"

namespace tls {

    class Socket : public ::Socket {
    public:
        SSL* startSSL(SSL_CTX* ctx);
        void stopSSL();

    protected:
        SSL* ssl = nullptr;
    };

}; // namespace tls

#endif // TLS_SOCKET_H
