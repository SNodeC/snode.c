#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <netinet/tcp.h>

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
                  std::cout << "OnConnect2" << std::endl;
                  SSL* ssl = cs->startSSL(this->ctx);
                  
                  std::cout << "OnConnect2" << std::endl;
                  
                  int err = SSL_connect(ssl);
                  
                  X509* cert = SSL_get_peer_certificate(ssl);
                  
                  std::cout << "OnConnect3" << std::endl;
                  
                  int sslerr = SSL_ERROR_NONE;
                  sslerr = SSL_get_error(ssl, err);
                  if (err < 1) {
                      std::cout << "OnConnect4" << std::endl;
                      sslerr = SSL_get_error(ssl, err);
                      std::cout << "OnConnect5" << std::endl;
                  }

                  if (sslerr != SSL_ERROR_NONE) {
                      std::cout << "OnConnect6" << std::endl;
                      cs->end();
                      std::cout << "OnConnect7" << std::endl;
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
              onDisconnect, onRead, onReadError, onWriteError) {
    }

}; // namespace tls
