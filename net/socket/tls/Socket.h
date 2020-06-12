#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/Socket.h"

namespace tls {

    class Socket : public ::Socket {
    public:
        bool startSSL(SSL_CTX* ctx);
        void stopSSL();

    protected:
        Socket();

    protected:
        SSL* ssl;
        int err;
    };

}; // namespace tls

#endif // SSLSOCKET_H
