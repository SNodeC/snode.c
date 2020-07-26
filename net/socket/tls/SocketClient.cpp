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
                  } else {
                      printf("Client does not have certificate.\n");
                  }
                  */
                      std::cout << "OnConnect8" << std::endl;
                      onConnect(cs);
                      std::cout << "OnConnect9" << std::endl;
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

}; // namespace tls
