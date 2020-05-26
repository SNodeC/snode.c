#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string.h>

#include <iostream>

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


void SocketSSLServer::listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM, const std::string& password, const std::function<void (int err)>& onError) {
    SocketServerBase<SocketSSLConnection>::listen(port, backlog, 
        [this, &certChain, &keyPEM, &password, onError] (int err) -> void {
            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_ssl_algorithms();
                
            ctx = SSL_CTX_new(TLS_server_method());
            if (!ctx) {
                ERR_print_errors_fp(stderr);
                err = ERR_get_error();
            } else {
                SSL_CTX_set_default_passwd_cb(ctx, SocketSSLServer::passwordCallback);
                SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
                if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                    ERR_print_errors_fp(stderr);
                    err = ERR_get_error();
                } else if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                    ERR_print_errors_fp(stderr);
                    err = ERR_get_error();
                } else if (!SSL_CTX_check_private_key(ctx)) {
                    err = ERR_get_error();
                }
            }
            
            onError(err);
        }
    );
}


int SocketSSLServer::passwordCallback(char *buf, int size, int rwflag, void *u) {
    ::strncpy(buf, (char*) u, size);
    buf[size - 1] = '\0';
    
    ::memset(u, 0, ::strlen((char*) u)); // garble password
    ::free(u);
    
    return ::strlen(buf);
}
