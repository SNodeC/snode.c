#include "ssl_utils.h"

#include <cstdlib> // for free
#include <cstring>
#include <openssl/err.h> // for ERR_peek_error
#include <openssl/ssl.h> // IWYU pragma: keep, for SSL_CTX_check_private_key, SSL_CTX_load_ve...
#include <string>
#include <utility> // for tuple_element<>::type

// IWYU pragma: no_include <openssl/ssl3.h>

namespace net::socket::tls {

#define SSL_VERIFY_FLAGS SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE

    static int password_callback(char* buf, int size, [[maybe_unused]] int rwflag, void* u) {
        strncpy(buf, static_cast<char*>(u), size);
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble password
        free(u);

        return ::strlen(buf);
    }

    unsigned long ssl_init_ctx(SSL_CTX* ctx, const std::map<std::string, std::any>& options, bool server) {
        unsigned long sslErr = 0;

        std::string certChain = "";
        std::string keyPEM = "";
        std::string password = "";
        std::string caFile = "";
        std::string caDir = "";
        bool useDefaultCADir = false;

        for (auto& [name, value] : options) {
            if (name == "certChain") {
                certChain = std::any_cast<const char*>(value);
            } else if (name == "keyPEM") {
                keyPEM = std::any_cast<const char*>(value);
            } else if (name == "password") {
                password = std::any_cast<const char*>(value);
            } else if (name == "caFile") {
                caFile = std::any_cast<const char*>(value);
            } else if (name == "caDir") {
                caDir = std::any_cast<const char*>(value);
            } else if (name == "useDefaultCADir") {
                useDefaultCADir = std::any_cast<bool>(value);
            }
        }

        if (ctx != nullptr) {
            if (!caFile.empty() || !caDir.empty()) {
                if (!SSL_CTX_load_verify_locations(ctx, !caFile.empty() ? caFile.c_str() : nullptr,
                                                   !caDir.empty() ? caDir.c_str() : nullptr)) {
                    sslErr = ERR_peek_error();
                }
            }
            if (sslErr == SSL_ERROR_NONE && useDefaultCADir) {
                if (!SSL_CTX_set_default_verify_paths(ctx)) {
                    sslErr = ERR_peek_error();
                }
            }
            if (server && sslErr == 0 && (useDefaultCADir || !caFile.empty() || !caDir.empty())) {
                SSL_CTX_set_verify(ctx, SSL_VERIFY_FLAGS, NULL);
            }
            if (sslErr == SSL_ERROR_NONE) {
                if (!certChain.empty()) {
                    if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                        sslErr = ERR_peek_error();
                    } else if (!keyPEM.empty()) {
                        if (!password.empty()) {
                            SSL_CTX_set_default_passwd_cb(ctx, password_callback);
                            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
                        }
                        if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                            sslErr = ERR_peek_error();
                        } else if (!SSL_CTX_check_private_key(ctx)) {
                            sslErr = ERR_peek_error();
                        }
                    }
                }
            }
        } else {
            sslErr = ERR_peek_error();
        }

        return sslErr;
    };

} // namespace net::socket::tls
