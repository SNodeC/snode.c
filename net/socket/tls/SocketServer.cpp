#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>
#include <string.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "socket/tls/SocketServer.h"


namespace tls {

    SocketServer::SocketServer(const std::function<void(::SocketConnection* cs)>& onConnect,
                               const std::function<void(::SocketConnection* cs)>& onDisconnect,
                               const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                               const std::function<void(::SocketConnection* cs, int errnum)>& onCsReadError,
                               const std::function<void(::SocketConnection* cs, int errnum)>& onCsWriteError)
        : SocketServerBase<tls::SocketConnection>(
              [this](::SocketConnection* cs) {
                  if (!dynamic_cast<tls::SocketConnection*>(cs)->startSSL(ctx)) {
                      Multiplexer::instance().getManagedReader().remove(dynamic_cast<tls::SocketConnection*>(cs));
                  }
                  this->onConnect(cs);
              },
              [this](::SocketConnection* cs) {
                  dynamic_cast<tls::SocketConnection*>(cs)->stopSSL();
                  this->onDisconnect(cs);
              },
              readProcessor, onCsReadError, onCsWriteError)
        , onConnect(onConnect)
        , onDisconnect(onDisconnect)
        , ctx(0) {
    }


    SocketServer* SocketServer::instance(const std::function<void(::SocketConnection* cs)>& onConnect,
                                         const std::function<void(::SocketConnection* cs)>& onDisconnect,
                                         const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                         const std::function<void(::SocketConnection* cs, int errnum)>& onCsReadError,
                                         const std::function<void(::SocketConnection* cs, int errnum)>& onCsWriteError) {
        return new SocketServer(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError);
    }


    SocketServer::~SocketServer() {
        if (ctx) {
            SSL_CTX_free(ctx);
        }
    }


    void SocketServer::listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM,
                              const std::string& password, const std::function<void(int err)>& onError) {
        SocketServerBase<SocketConnection>::listen(port, backlog, [this, &certChain, &keyPEM, &password, onError](int err) -> void {
            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_ssl_algorithms();

            ctx = SSL_CTX_new(TLS_server_method());
            if (!ctx) {
                ERR_print_errors_fp(stderr);
                err = ERR_get_error();
            } else {
                SSL_CTX_set_default_passwd_cb(ctx, SocketServer::passwordCallback);
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
        });
    }


    int SocketServer::passwordCallback(char* buf, int size, int rwflag, void* u) {
        ::strncpy(buf, (char*) u, size);
        buf[size - 1] = '\0';

        ::memset(u, 0, ::strlen((char*) u)); // garble password
        ::free(u);

        return ::strlen(buf);
    }

}; // namespace tls
