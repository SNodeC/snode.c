#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketSSLServer.h"


SocketSSLServer::SocketSSLServer(const std::function<void (SocketConnection* cs)>& onConnect,
                                 const std::function<void (SocketConnection* cs)>& onDisconnect,
                                 const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                                 const std::function<void (int errnum)>& onCsReadError,
                                 const std::function<void (int errnum)>& onCsWriteError) :
    SocketServerBase<SocketSSLConnection>(
        [this] (SocketConnection* cs) {
            dynamic_cast<SocketSSLConnection*>(cs)->startSSL(ctx);
            this->onConnect(cs);
        }, onDisconnect, readProcessor, onCsReadError, onCsWriteError), onConnect(onConnect), ctx(0) {
}

SocketSSLServer* SocketSSLServer::instance(const std::function<void (SocketConnection* cs)>& onConnect,
                                           const std::function<void (SocketConnection* cs)>& onDisconnect,
                                           const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                                           const std::function<void (int errnum)>& onCsReadError,
                                           const std::function<void (int errnum)>& onCsWriteError) {
    return new SocketSSLServer(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError);
}


SocketSSLServer::~SocketSSLServer() {
    if (ctx) {
        SSL_CTX_free(ctx);
    }
}


void SocketSSLServer::listen(in_port_t port, int backlog, const std::string& cert, const std::string& key, const std::string& password, const std::function<void (int err)>& onError) {
    SocketServerBase<SocketSSLConnection>::listen(port, backlog, 
        [this, &cert, &key, &password, onError] (int err) -> void {
            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_ssl_algorithms();
                
            ctx = SSL_CTX_new(TLS_server_method());
            if (!ctx) {
                ERR_print_errors_fp(stderr);
                err = ERR_get_error();
            } else if (SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                err = ERR_get_error();
            } else if (SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                err = ERR_get_error();
            } else if (!SSL_CTX_check_private_key(ctx)) {
//                fprintf(stderr,"Private key does not match the certificate public key\n");
                err = ERR_get_error();
            }
            
            onError(err);
        }
    );
}


void SocketSSLServer::readEvent() {
    SocketServerBase<SocketSSLConnection>::readEvent();
}

