#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SSLSocket.h"


SSLSocket::SSLSocket() {}


SSLSocket::SSLSocket(int fd) : SocketBase(), ssl(0) {
    this->setFd(fd);
}


SSLSocket::~SSLSocket() {
    SSL_shutdown(ssl);
    SSL_free(ssl);
}


void SSLSocket::setFd(int fd) {
    SocketBase::setFd(fd);
}


void SSLSocket::setCTX(SSL_CTX* ctx) {
    this->ssl = SSL_new(ctx);
    SSL_set_fd(ssl, getFd());
    err = SSL_accept(this->ssl);
    
/*
     client_cert = SSL_get_peer_certificate (ssl);
     if (client_cert != NULL) {
         printf ("Client certificate:\n");
         
         str = X509_NAME_oneline (X509_get_subject_name (client_cert), 0, 0);
         CHK_NULL(str);
         printf ("\t subject: %s\n", str);
         OPENSSL_free (str);
         
         str = X509_NAME_oneline (X509_get_issuer_name  (client_cert), 0, 0);
         CHK_NULL(str);
         printf ("\t issuer: %s\n", str);
         OPENSSL_free (str);
         
         // We could do all sorts of certificate verification stuff here before deallocating the certificate.
         
         X509_free (client_cert);
    } else {
        printf ("Client does not have certificate.\n");
    }
*/
}


ssize_t SSLSocket::socketRecv(void *buf, size_t len, int) {
    ssize_t ret = err;
    
    if (err > 0) {
        return ::SSL_read(ssl, buf, len);
    }
    
    return ret;
}


ssize_t SSLSocket::socketSend(const void *buf, size_t len, int) {
    ssize_t ret = err;
    
    if (err > 0) {
        ret = ::SSL_write(ssl, buf, len);
    }
    
    return ret;
}
