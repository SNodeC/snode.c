#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketClient.h"


namespace tls {

    SocketClient::SocketClient(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                               const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                               const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                               const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError)
        : ::SocketClient<tls::SocketConnection>(
              [this, onConnect](tls::SocketConnection* cs) -> void {
                  SSL* ssl = cs->startSSL(this->ctx);

                  int err = 0;
                  do {
                      err = SSL_connect(ssl);
                  } while (SSL_get_error(ssl, err) == SSL_ERROR_WANT_READ || SSL_get_error(ssl, err) == SSL_ERROR_WANT_WRITE);

                  if (SSL_get_error(ssl, err) != SSL_ERROR_NONE) {
                      cs->end();
                  } else {
                      /*
                       *   X509* client_cert = SSL_get_peer_certificate(ssl);
                       *   if (client_cert != NULL) {
                       *       printf("Client certificate:\n");
                       *
                       *       char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
                       *       printf("\t subject: %s\n", str);
                       *       OPENSSL_free(str);
                       *
                       *       str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
                       *       printf("\t issuer: %s\n", str);
                       *       OPENSSL_free(str);
                       *
                       *       // We could do all sorts of certificate verification stuff here before deallocating the certificate.
                       *
                       *       X509_free(client_cert);
                       *   } else {
                       *       printf("Client does not have certificate.\n");
                       *   }
                       */
                      onConnect(cs);
                  }
              },
              [onDisconnect](tls::SocketConnection* cs) -> void {
                  cs->stopSSL();
                  onDisconnect(cs);
              },
              onRead, onReadError, onWriteError) {
    }

    SocketClient::~SocketClient() {
        if (ctx) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }

    void SocketClient::connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                               const InetAddress& localAddress) {
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_ssl_algorithms();

        ctx = SSL_CTX_new(TLS_client_method());
        [[maybe_unused]] unsigned long sslErr = 0;
        if (!ctx) {
            ERR_print_errors_fp(stderr);
            sslErr = ERR_get_error();
            onError(10); // TODO: Error handling
        } else {         /*
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
        }*/
        }
        ::SocketClient<tls::SocketConnection>::connect(host, port, onError, localAddress);
    }

}; // namespace tls
