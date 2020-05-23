#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketBase.h"


class SSLSocket : public SocketBase {
public:
    virtual void setFd(int fd);

    virtual ~SSLSocket();

protected:
    SSLSocket();
    SSLSocket(int fd);

    ssize_t socketRecv(void *buf, size_t len, int flags);
    ssize_t socketSend(const void *buf, size_t len, int flags);

private:
    static const SSL_METHOD* init1();
    static SSL_CTX* init2();
    static SSL_CTX* ctx;
    static const SSL_METHOD* meth;
    SSL* ssl;
    int err;
};

#endif // SSLSOCKET_H
