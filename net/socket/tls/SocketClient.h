#ifndef TLS_SOCKETCLIENT_H
#define TLS_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketClient.h"
#include "socket/tls/SocketConnection.h"

namespace tls {

    class SocketClient : public ::SocketClient<tls::SocketConnection> {
    public:
        SocketClient(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                     const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError);

        ~SocketClient();

        void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                     const InetAddress& localAddress = InetAddress()) override {
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

        void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, in_port_t lPort) override {
            connect(host, port, onError, InetAddress(lPort));
        }

        void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                     const std::string lHost) override {
            connect(host, port, onError, InetAddress(lHost));
        }

        void connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError, const std::string lHost,
                     in_port_t lPort) override {
            connect(host, port, onError, InetAddress(lHost, lPort));
        }

    private:
        SSL_CTX* ctx;

    private:
        std::function<void(tls::SocketConnection* cs)> onConnect;
        std::function<void(tls::SocketConnection* cs)> onDisconnect;
        std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)> onRead;
        std::function<void(tls::SocketConnection* cs, int errnum)> onReadError;
        std::function<void(tls::SocketConnection* cs, int errnum)> onWriteError;
    };

} // namespace tls

#endif // TLS_SOCKETCLIENT_H
