/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib> // for free
#include <cstring>
#include <openssl/err.h> // for ERR_peek_error
#include <openssl/ssl.h> // IWYU pragma: keep, for SSL_CTX_check_private_key, SSL_CTX_load_ve...
#include <string>
#include <utility> // for tuple_element<>::type

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "ssl_utils.h"

namespace net::socket::stream::tls {

#define SSL_VERIFY_FLAGS SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE

    static int password_callback(char* buf, int size, int, void* u) {
        strncpy(buf, static_cast<char*>(u), size);
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble password
        free(u);

        return ::strlen(buf);
    }

    static int verify_callback([[maybe_unused]] int preverify_ok, [[maybe_unused]] X509_STORE_CTX* ctx) {
        return 1;
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
                if (!SSL_CTX_load_verify_locations(
                        ctx, !caFile.empty() ? caFile.c_str() : nullptr, !caDir.empty() ? caDir.c_str() : nullptr)) {
                    sslErr = ERR_peek_error();
                }
            }
            if (sslErr == SSL_ERROR_NONE && useDefaultCADir) {
                if (!SSL_CTX_set_default_verify_paths(ctx)) {
                    sslErr = ERR_peek_error();
                }
            }
            if (server && sslErr == 0 && (useDefaultCADir || !caFile.empty() || !caDir.empty())) {
                SSL_CTX_set_verify_depth(ctx, 5);
                SSL_CTX_set_verify(ctx, SSL_VERIFY_FLAGS, verify_callback);
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

    void ssl_log(const std::string& message, int sslErr) {
        switch (sslErr) {
            case SSL_ERROR_NONE:
            case SSL_ERROR_ZERO_RETURN:
                ssl_log_info(message);
                break;
            case SSL_ERROR_SYSCALL:
                if (errno != 0) {
                    ssl_log_warning(message);
                } else {
                    ssl_log_info(message + ". Client disconnect unexpectedly");
                }
                break;
            default:
                ssl_log_warning(message);
                break;
        };
    }

    void ssl_log_error(const std::string& message) {
        PLOG(ERROR) << message;

        long errorCode;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(ERROR) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_warning(const std::string& message) {
        PLOG(WARNING) << message;

        long errorCode;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(WARNING) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_info(const std::string& message) {
        PLOG(INFO) << message;

        long errorCode;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(INFO) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

} // namespace net::socket::stream::tls

#ifdef WITH_VERIFY
char buf[256];
X509* err_cert;
int err, depth;
SSL* ssl;

err_cert = X509_STORE_CTX_get_current_cert(ctx);
err = X509_STORE_CTX_get_error(ctx);
depth = X509_STORE_CTX_get_error_depth(ctx);

/*
 * Retrieve the pointer to the SSL of the connection currently treated
 * and the application specific data stored into the SSL object.
 */
ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));

X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

/*
 * Catch a too long certificate chain. The depth limit set using
 * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
 * that whenever the "depth>verify_depth" condition is met, we
 * have violated the limit and want to log this error condition.
 * We must do it here, because the CHAIN_TOO_LONG error would not
 * be found explicitly; only errors introduced by cutting off the
 * additional certificates would be logged.
 */
if (depth > SSL_get_verify_depth(ssl)) {
    preverify_ok = 0;
    err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
    X509_STORE_CTX_set_error(ctx, err);
}
if (!preverify_ok) {
    LOG(ERROR) << "Certificate verification error";
    LOG(ERROR) << "|-- error code: " << err;
    LOG(ERROR) << "|-- message: " << X509_verify_cert_error_string(err);
    LOG(ERROR) << "|-- depth: " << depth;
    LOG(ERROR) << "|-- subject name: " << buf;
} else if (true) {
    LOG(INFO) << "Certificate";
    LOG(INFO) << "|-- depth: " << depth;
    LOG(INFO) << "|-- subject name: " << buf;
}

/*
 * At this point, err contains the last verification error. We can use
 * it for something special
 */
if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
    X509* client_cert = SSL_get_peer_certificate(ssl);
    X509_NAME_oneline(X509_get_issuer_name(client_cert), buf, 256);
    LOG(ERROR) << "|-- issuer: " << buf;
}

//        if (mydata->always_continue)
return 1;
//        else
//            return preverify_ok;
#endif // WITH_VERIFY
