#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h>

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SSLSocket.h"
#include "Descriptor.h"


const SSL_METHOD* SSLSocket::meth = SSLSocket::init1();
SSL_CTX* SSLSocket::ctx = SSLSocket::init2();


const SSL_METHOD* SSLSocket::init1() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();
    return TLS_server_method();
}

#define CERTF "/home/voc/projects/ServerVoc/certs/test-cert.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/test-cert.pem"


SSL_CTX* SSLSocket::init2() {
    SSL_CTX* ctx = SSL_CTX_new(SSLSocket::meth);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(2);
    }
    
    if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(3);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(4);
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr,"Private key does not match the certificate public key\n");
        exit(5);
    }
    
    return ctx;
}


SSLSocket::SSLSocket() {}


SSLSocket::SSLSocket(int fd) : SocketBase() {
    this->setFd(fd);
}


SSLSocket::~SSLSocket() {
    SSL_shutdown(ssl);
    SSL_free(ssl);
}


void SSLSocket::setFd(int fd) {
    SocketBase::setFd(fd);
    
    ssl = SSL_new(SSLSocket::ctx);
    SSL_set_fd(ssl, fd);
    err = SSL_accept(ssl);
    
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


ssize_t SSLSocket::recv(void *buf, size_t len, int flags) {
    return ::SSL_read(ssl, buf, len);
//    return ::recv(this->getFd(), buf, len, flags);
}


ssize_t SSLSocket::send(const void *buf, size_t len, int flags) {
    return ::SSL_write(ssl, buf, len);
//    return ::send(this->getFd(), buf, len, flags);
}
