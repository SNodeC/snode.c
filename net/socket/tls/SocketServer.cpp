#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketServer.h"


namespace tls {

    SocketServer::SocketServer(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                               const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                               const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                               const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError)
        : ::SocketServer<tls::SocketConnection>(
              [this, onConnect](tls::SocketConnection* cs) {
                  if (!cs->startSSL(this->ctx)) {
                      cs->end();
                  }
                  onConnect(cs);
              },
              [onDisconnect](tls::SocketConnection* cs) {
                  cs->stopSSL();
                  onDisconnect(cs);
              },
              onRead, onReadError, onWriteError)
        , ctx(nullptr) {
    }


    SocketServer::~SocketServer() {
        if (ctx) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }


    void SocketServer::listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM,
                              const std::string& password, const std::function<void(int err)>& onError) {
        ::SocketServer<tls::SocketConnection>::listen(port, backlog, [this, &certChain, &keyPEM, &password, onError](int err) -> void {
            if (!err) {
                SSL_load_error_strings();
                SSL_library_init();
                OpenSSL_add_ssl_algorithms();

                ctx = SSL_CTX_new(TLS_server_method());
                unsigned long sslErr{};
                if (!ctx) {
                    ERR_print_errors_fp(stderr);
                    sslErr = ERR_get_error();
                } else {
                    SSL_CTX_set_default_passwd_cb(ctx, SocketServer::passwordCallback);
                    SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
                    if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                        ERR_print_errors_fp(stderr);
                        sslErr = ERR_get_error();
                    } else if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                        ERR_print_errors_fp(stderr);
                        sslErr = ERR_get_error();
                    } else if (!SSL_CTX_check_private_key(ctx)) {
                        sslErr = ERR_get_error();
                    }
                }
                onError(-ERR_GET_REASON(sslErr));
            } else {
                onError(err);
            }
        });
    }


    int SocketServer::passwordCallback(char* buf, int size, int rwflag, void* u) {
        strncpy(buf, static_cast<char*>(u), size);
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble password
        free(u);

        return ::strlen(buf);
    }

}; // namespace tls
