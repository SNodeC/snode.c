/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/tls/ssl_utils.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>  // for errno
#include <cstdlib> // for free
#include <cstring>
#include <openssl/err.h>  // for ERR_peek_error
#include <openssl/ssl.h>  // IWYU pragma: keep
#include <openssl/x509.h> // for X509_NAME_oneline, X509_STORE_CTX_get_current_cert, X509_STORE_CTX_get_error, X509_STORE_CTX_get_error_depth, X509_get_subject_name, X509_verify_cert_error_string, X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
#include <string>
#include <utility> // for tuple_element<>::type

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

#define SSL_VERIFY_FLAGS (SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE)

    static int password_callback(char* buf, int size, int, void* u) {
        strncpy(buf, static_cast<char*>(u), static_cast<std::size_t>(size));
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble Password
        free(u);

        return static_cast<int>(::strlen(buf));
    }

    static int verify_callback(int preverify_ok, [[maybe_unused]] X509_STORE_CTX* ctx) {
        char buf[256];
        X509* err_cert;
        int err = 0;
        int depth = 0;

        err_cert = X509_STORE_CTX_get_current_cert(ctx);
        err = X509_STORE_CTX_get_error(ctx);
        depth = X509_STORE_CTX_get_error_depth(ctx);

        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

        if (!preverify_ok) {
            LOG(INFO) << "verify_error:num=" << err << ":" << X509_verify_cert_error_string(err) << ":depth=" << depth << ":" << buf;
        } else {
            LOG(INFO) << "depth=" << depth << ":" << buf;
        }

        /*
         * At this point, err contains the last verification error. We can use
         * it for something special
         */
        /*
                if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
                    X509_NAME_oneline(X509_get_issuer_name(err_cert), buf, 256);
                    printf("issuer= %s\n", buf);
                }
        */

        return preverify_ok;
    }

    SSL_CTX* ssl_ctx_new(const std::map<std::string, std::any>& options, bool server) {
        static int sslSessionCtxId = 1;

        std::string certChain;
        std::string certChainKey;
        std::string password;
        std::string caFile;
        std::string caDir;
        bool useDefaultCaDir = false;

        for (const auto& [name, value] : options) { // cppcheck-suppress unassignedVariable
            if (name == "CertChain") {
                certChain = std::any_cast<const char*>(value);
            } else if (name == "CertChainKey") {
                certChainKey = std::any_cast<const char*>(value);
            } else if (name == "Password") {
                password = std::any_cast<const char*>(value);
            } else if (name == "CaFile") {
                caFile = std::any_cast<const char*>(value);
            } else if (name == "CaDir") {
                caDir = std::any_cast<const char*>(value);
            } else if (name == "UseDefaultCaDir") {
                useDefaultCaDir = std::any_cast<bool>(value);
            }
        }

        SSL_CTX* ctx = SSL_CTX_new(server ? TLS_server_method() : TLS_client_method());

        if (ctx != nullptr) {
            bool sslErr = false;

            if (server) {
                SSL_CTX_set_session_id_context(ctx, (const unsigned char*) &sslSessionCtxId, sizeof(sslSessionCtxId));
                sslSessionCtxId++;
            }
            if (!caFile.empty() || !caDir.empty()) {
                if (!SSL_CTX_load_verify_locations(
                        ctx, !caFile.empty() ? caFile.c_str() : nullptr, !caDir.empty() ? caDir.c_str() : nullptr)) {
                    ssl_log_error("Can not load CA file or non default CA directory");
                    sslErr = true;
                }
            }
            if (!sslErr && useDefaultCaDir) {
                if (!SSL_CTX_set_default_verify_paths(ctx)) {
                    ssl_log_error("Can not load default CA directory");
                    sslErr = true;
                }
            }
            if (!sslErr) {
                if (server && (useDefaultCaDir || !caFile.empty() || !caDir.empty())) {
                    SSL_CTX_set_verify_depth(ctx, 5);
                    SSL_CTX_set_verify(ctx, SSL_VERIFY_FLAGS, verify_callback);
                }
                if (!certChain.empty()) {
                    if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) != 1) {
                        ssl_log_error("Can not load certificate chain");
                        sslErr = true;
                    } else if (!certChainKey.empty()) {
                        if (!password.empty()) {
                            SSL_CTX_set_default_passwd_cb(ctx, password_callback);
                            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
                        }
                        if (SSL_CTX_use_PrivateKey_file(ctx, certChainKey.c_str(), SSL_FILETYPE_PEM) != 1) {
                            errno = EINVAL;
                            ssl_log_error("Can not load private key");
                            sslErr = true;
                        } else if (!SSL_CTX_check_private_key(ctx)) {
                            errno = EINVAL;
                            ssl_log_error("Private key not consistent with CA files");
                            sslErr = true;
                        }
                    }
                }
            }
            if (sslErr) {
                SSL_CTX_free(ctx);
                ctx = nullptr;
            }
        } else {
            ssl_log_error("SSL CTX not created");
        }

        return ctx;
    }

    void ssl_ctx_free(SSL_CTX* ctx) {
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
        }
    }

    void ssl_set_sni(SSL* ssl, std::map<std::string, std::any>& options) {
        if (options.contains("SNI")) {
            SSL_set_tlsext_host_name(ssl, std::any_cast<const char*>(options["SNI"]));
        }
    }

    void ssl_log(const std::string& message, int sslErr) {
        switch (sslErr) {
            case SSL_ERROR_NONE:
                [[fallthrough]];
            case SSL_ERROR_ZERO_RETURN:
                ssl_log_info(message);
                break;
            case SSL_ERROR_SYSCALL:
                if (errno != 0) {
                    ssl_log_error(message);
                }
                break;
            default:
                ssl_log_warning(message);
                break;
        };
    }

    void ssl_log_error(const std::string& message) {
        PLOG(ERROR) << message;

        unsigned long errorCode;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(ERROR) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_warning(const std::string& message) {
        LOG(WARNING) << message;

        unsigned long errorCode;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(WARNING) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_info(const std::string& message) {
        PLOG(INFO) << message;

        unsigned long errorCode;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(INFO) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

} // namespace core::socket::stream::tls
