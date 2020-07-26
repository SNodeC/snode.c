#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // for SSL_accept, SSL_free, SSL_get_error, SSL_new

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/Socket.h"


namespace tls {

    SSL* Socket::startSSL(SSL_CTX* ctx) {
        this->ssl = SSL_new(ctx);
        SSL_set_fd(ssl, getFd());

        return ssl;
    }


    void Socket::stopSSL() {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
}; // namespace tls
