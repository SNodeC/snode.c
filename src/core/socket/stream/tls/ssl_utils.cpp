/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/tls/ssl_utils.h"

#include "log/Logger.h"
#include "net/config/ConfigTlsClient.h" // IWYU pragma: keep
#include "net/config/ConfigTlsServer.h" // IWYU pragma: keep

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <string>

// IWYU pragma: no_include <openssl/ssl3.h>
// IWYU pragma: no_include <bits/utility.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
using ssl_option_t = uint64_t;
#else
using ssl_option_t = uint32_t;
#endif

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

    static int verify_callback(int preverify_ok, X509_STORE_CTX* ctx) {
        char buf[256];
        X509* err_cert = nullptr;
        int err = 0;
        int depth = 0;

        err_cert = X509_STORE_CTX_get_current_cert(ctx);
        err = X509_STORE_CTX_get_error(ctx);
        depth = X509_STORE_CTX_get_error_depth(ctx);

        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

        if (!preverify_ok) {
            LOG(INFO) << "verify_error:num=" << err << ": " << X509_verify_cert_error_string(err) << ":depth=" << depth << ": " << buf;
        } else {
            LOG(TRACE) << "depth=" << depth << ": " << buf;
        }

        /*
         * At this point, err contains the last verification error. We can use
         * it for something special
         */

        if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
            X509_NAME_oneline(X509_get_issuer_name(err_cert), buf, 256);
            LOG(WARNING) << "no issuer certificate for issuer= " << buf;
        }

        return preverify_ok;
    }

    struct SslConfig {
        explicit SslConfig(bool server)
            : server(server) {
        }
        SslConfig(bool server, const std::shared_ptr<net::config::ConfigTls>& configTls)
            : certChain(configTls->getCertChain())
            , certChainKey(configTls->getCertKey())
            , password(configTls->getCertKeyPassword())
            , caFile(configTls->getCaCertFile())
            , caDir(configTls->getCaCertDir())
            , useDefaultCaDir(configTls->getUseDefaultCaCertDir())
            , cipherList(configTls->getCipherList())
            , sslOptions(static_cast<ssl_option_t>(configTls->getSslTlsOptions()))
            , server(server) {
        }

        std::string certChain;
        std::string certChainKey;
        std::string password;
        std::string caFile;
        std::string caDir;
        bool useDefaultCaDir = false;
        std::string cipherList;
        ssl_option_t sslOptions = 0;
        bool server = false;
    };

    SSL_CTX* ssl_ctx_new(const SslConfig& sslConfig);
    SSL_CTX* ssl_ctx_new(const SslConfig& sslConfig) {
        static int sslSessionCtxId = 1;

        SSL_CTX* ctx = SSL_CTX_new(sslConfig.server ? TLS_server_method() : TLS_client_method());

        if (ctx != nullptr) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
            SSL_CTX_set_options(ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif
            SSL_CTX_set_read_ahead(ctx, 1);

            SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
            SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

            bool sslErr = false;

            if (sslConfig.server) {
                SSL_CTX_set_session_id_context(ctx, reinterpret_cast<const unsigned char*>(&sslSessionCtxId), sizeof(sslSessionCtxId));
                sslSessionCtxId++;
            }
            if (!sslConfig.caFile.empty() || !sslConfig.caDir.empty()) {
                if (!SSL_CTX_load_verify_locations(ctx,
                                                   !sslConfig.caFile.empty() ? sslConfig.caFile.c_str() : nullptr,
                                                   !sslConfig.caDir.empty() ? sslConfig.caDir.c_str() : nullptr)) {
                    ssl_log_error("Can not load CA file or non default CA directory: CA-File " + sslConfig.caFile + " : CA-Dir " +
                                  sslConfig.caDir);
                    sslErr = true;
                }
            }
            if (!sslErr && sslConfig.useDefaultCaDir) {
                if (!SSL_CTX_set_default_verify_paths(ctx)) {
                    ssl_log_error("Can not load default CA directory");
                    sslErr = true;
                }
            }
            if (!sslErr) {
                if (sslConfig.server && (sslConfig.useDefaultCaDir || !sslConfig.caFile.empty() || !sslConfig.caDir.empty())) {
                    SSL_CTX_set_verify_depth(ctx, 5);
                    SSL_CTX_set_verify(ctx, SSL_VERIFY_FLAGS, verify_callback);
                }
                if (!sslConfig.certChain.empty()) {
                    if (SSL_CTX_use_certificate_chain_file(ctx, sslConfig.certChain.c_str()) != 1) {
                        ssl_log_error("Can not load certificate chain: " + sslConfig.certChain);
                        sslErr = true;
                    } else if (!sslConfig.certChainKey.empty()) {
                        if (!sslConfig.password.empty()) {
                            SSL_CTX_set_default_passwd_cb(ctx, password_callback);
                            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(sslConfig.password.c_str()));
                        }
                        if (SSL_CTX_use_PrivateKey_file(ctx, sslConfig.certChainKey.c_str(), SSL_FILETYPE_PEM) != 1) {
                            ssl_log_error("Can not load private key: " + sslConfig.certChainKey);
                            sslErr = true;
                        } else if (!SSL_CTX_check_private_key(ctx)) {
                            ssl_log_error("Private key not consistent with CA files: " + sslConfig.certChainKey);
                            sslErr = true;
                        }
                    }
                }
            }
            if (!sslErr) {
                if (sslConfig.sslOptions != 0) {
                    SSL_CTX_set_options(ctx, sslConfig.sslOptions);
                }
                if (!sslConfig.cipherList.empty()) {
                    SSL_CTX_set_cipher_list(ctx, sslConfig.cipherList.c_str());
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

    SSL_CTX* ssl_ctx_new(const std::shared_ptr<net::config::ConfigTlsServer>& configTls) {
        return ssl_ctx_new(SslConfig(true, configTls));
    }

    SSL_CTX* ssl_ctx_new(const std::shared_ptr<net::config::ConfigTlsClient>& configTls) {
        return ssl_ctx_new(SslConfig(false, configTls));
    }

    SSL_CTX* ssl_ctx_new(const std::map<std::string, std::any>& sniCert) {
        SslConfig sslConfig(true);

        for (auto& [key, value] : sniCert) {
            if (key == "CertChain") {
                sslConfig.certChain = std::any_cast<const std::string&>(value);
            } else if (key == "CertKey") {
                sslConfig.certChainKey = std::any_cast<const std::string&>(value);
            } else if (key == "CertKeyPassword") {
                sslConfig.password = std::any_cast<const std::string&>(value);
            } else if (key == "CaCertFile") {
                sslConfig.caFile = std::any_cast<const std::string&>(value);
            } else if (key == "CaCertDir") {
                sslConfig.caDir = std::any_cast<const std::string&>(value);
            } else if (key == "UseDefaultCaDir") {
                sslConfig.useDefaultCaDir = std::any_cast<bool>(value);
            } else if (key == "CipherList") {
                sslConfig.cipherList = std::any_cast<const std::string&>(value);
            } else if (key == "SslOptions") {
                sslConfig.sslOptions = std::any_cast<ssl_option_t>(value);
            }
        }

        return ssl_ctx_new(sslConfig);
    }

    SSL_CTX* ssl_set_ssl_ctx(SSL* ssl, SSL_CTX* sslCtx) {
        SSL_CTX* newSslCtx = SSL_set_SSL_CTX(ssl, sslCtx);
        SSL_clear_options(ssl, 0xFFFFFFFFL);
        SSL_set_options(ssl, SSL_CTX_get_options(sslCtx));
        SSL_set_verify(ssl, SSL_CTX_get_verify_mode(sslCtx), SSL_CTX_get_verify_callback(sslCtx));
        SSL_set_verify_depth(ssl, SSL_CTX_get_verify_depth(sslCtx));
        SSL_set_mode(ssl, SSL_CTX_get_mode(sslCtx));

        return newSslCtx;
    }

    void ssl_ctx_free(SSL_CTX* ctx) {
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
        }
    }

    void ssl_set_sni(SSL* ssl, const std::shared_ptr<net::config::ConfigTlsClient>& configTls) {
        if (!configTls->getSni().empty()) {
            SSL_set_tlsext_host_name(ssl, configTls->getSni().data());
        }
    }

    std::set<std::string> ssl_get_sans(SSL_CTX* sslCtx) {
        std::set<std::string> sans;

        if (sslCtx != nullptr) {
            X509* x509 = SSL_CTX_get0_certificate(sslCtx);
            if (x509 != nullptr) {
                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(x509, NID_subject_alt_name, nullptr, nullptr));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);

                for (int32_t i = 0; i < altNameCount; ++i) {
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                    if (generalName->type == GEN_DNS || generalName->type == GEN_URI || generalName->type == GEN_EMAIL) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        VLOG(2) << "SSL_CTX for domain '" << subjectAltName << "' installed";
                        sans.insert(subjectAltName);
                    }
                }
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);
            }
        }

        return sans;
    }

    // From: https://www.bit-hive.com/documents/openssl-tutorial/
    std::string ssl_get_servername_from_client_hello(SSL* ssl) {
        const unsigned char* ext;
        size_t ext_len;
        size_t p = 0;
        size_t server_name_list_len;
        size_t server_name_len;

        if (!SSL_client_hello_get0_ext(ssl, TLSEXT_TYPE_server_name, &ext, &ext_len)) {
            return "";
        }

        /* length (2 bytes) + type (1) + length (2) + server name (1+) */
        if (ext_len < 6) {
            return "";
        }

        /* Fetch Server Name list length */
        server_name_list_len = static_cast<size_t>((ext[p] << 8) + ext[p + 1]);
        p += 2;
        if (p + server_name_list_len != ext_len) {
            return "";
        }

        /* Fetch Server Name Type */
        if (ext[p] != TLSEXT_NAMETYPE_host_name) {
            return "";
        }
        p++;

        /* Fetch Server Name Length */
        server_name_len = static_cast<size_t>((ext[p] << 8) + ext[p + 1]);
        p += 2;
        if (p + server_name_len != ext_len) {
            return "";
        }

        /* ext_len >= 6 && p == 5 */

        /* Finally fetch Server Name */

        return std::string(reinterpret_cast<const char*>(ext + p), ext_len - p);
    }

    void ssl_log(const std::string& message, int sslErr) {
        int errnum = errno;
        switch (sslErr) {
            case SSL_ERROR_NONE:
                [[fallthrough]];
            case SSL_ERROR_ZERO_RETURN:
                ssl_log_info(message);
                break;
            case SSL_ERROR_SYSCALL:
                if (errno != 0) {
                    ssl_log_error(message);
                } else {
                    ssl_log_info(message);
                }
                break;
            default:
                ssl_log_warning(message);
                break;
        }
        errno = errnum;
    }

    void ssl_log_error(const std::string& message) {
        PLOG(ERROR) << message;

        unsigned long errorCode = 0;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(ERROR) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_warning(const std::string& message) {
        LOG(WARNING) << message;

        unsigned long errorCode = 0;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(WARNING) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_info(const std::string& message) {
        PLOG(INFO) << message;

        unsigned long errorCode = 0;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(INFO) << "|-- with SSL " << ERR_error_string(errorCode, nullptr);
        }
    }

} // namespace core::socket::stream::tls
