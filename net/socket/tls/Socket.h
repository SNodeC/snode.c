#ifndef TLS_SOCKET_H
#define TLS_SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

//#include <openssl/crypto.h>
//#include <openssl/err.h>
#include <openssl/ossl_typ.h> // for SSL, SSL_CTX
//#include <openssl/pem.h>
//#include <openssl/rsa.h>
//#include <openssl/ssl.h>
//#include <openssl/x509.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/Socket.h"


namespace tls {

    class Socket : public ::Socket {
    public:
        bool startSSL(SSL_CTX* ctx);
        void stopSSL();

    protected:
        SSL* ssl{nullptr};
        int err{0};
    };

}; // namespace tls

#endif // TLS_SOCKET_H
