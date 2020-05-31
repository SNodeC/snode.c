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

#include "Socket.h"

namespace ssl {

class SocketSSL : public Socket {
public:
    void startSSL(SSL_CTX* ctx);

    virtual ~SocketSSL();

protected:
    SocketSSL() = default;
    SocketSSL(int fd);

    ssize_t socketRecv(void* buf, size_t len, int flags);
    ssize_t socketSend(const void* buf, size_t len, int flags);

private:
    SSL* ssl;
    int err;
};

};

#endif // SSLSOCKET_H
