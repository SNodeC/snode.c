/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    static int password_callback(char* buf, int size, [[maybe_unused]] int a, void* u) {
        strncpy(buf, static_cast<char*>(u), static_cast<std::size_t>(size));
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble Password
        free(u);

        return static_cast<int>(std::strlen(buf));
    }

    static int verify_callback(int preverify_ok, X509_STORE_CTX* ctx) {
        SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));

        std::string connectionName = *static_cast<std::string*>(SSL_get_ex_data(ssl, 0));

        X509* curr_cert = X509_STORE_CTX_get_current_cert(ctx);
        const int depth = X509_STORE_CTX_get_error_depth(ctx);

        char subjectName[256];
        X509_NAME_oneline(X509_get_subject_name(curr_cert), subjectName, 256);

        char issuerName[256];
        X509_NAME_oneline(X509_get_issuer_name(curr_cert), issuerName, 256);

        if (preverify_ok != 0) {
            LOG(DEBUG) << connectionName << ": SSL/TLS verify success at depth=" << depth;
            LOG(DEBUG) << "   Issuer: " << issuerName;
            LOG(DEBUG) << "  Subject: " << subjectName;
        } else {
            const int err = X509_STORE_CTX_get_error(ctx);

            /*
             * At this point, err contains the last verification error. We can use
             * it for something special
             */

            LOG(DEBUG) << connectionName << ": SSL/TLS verify error at depth=" << depth << ": " << X509_verify_cert_error_string(err);
            LOG(DEBUG) << "   Issuer: " << issuerName;
            LOG(DEBUG) << "  Subject: " << subjectName;
        }

        return preverify_ok;
    }

    SslConfig::SslConfig(bool server)
        : server(server) {
    }

    SSL_CTX* ssl_ctx_new(const SslConfig& sslConfig) {
        static int sslSessionCtxId = 1;

        SSL_CTX* ctx = SSL_CTX_new(sslConfig.server ? TLS_server_method() : TLS_client_method());

        if (ctx != nullptr) {
            SSL_CTX_set_read_ahead(ctx, 1);

            SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
            SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

            bool sslErr = false;

            if (sslConfig.server) {
                SSL_CTX_set_session_id_context(ctx, reinterpret_cast<const unsigned char*>(&sslSessionCtxId), sizeof(sslSessionCtxId));
                sslSessionCtxId++;
            }
            if (!sslConfig.caCert.empty() || !sslConfig.caCertDir.empty()) {
                if (SSL_CTX_load_verify_locations(ctx,
                                                  !sslConfig.caCert.empty() ? sslConfig.caCert.c_str() : nullptr,
                                                  !sslConfig.caCertDir.empty() ? sslConfig.caCertDir.c_str() : nullptr) == 0) {
                    ssl_log_error(sslConfig.instanceName + " SSL/TLS: CA certificate error loading file '" + sslConfig.caCert + "', dir '" +
                                  sslConfig.caCertDir + "'");
                    sslErr = true;
                } else {
                    if (!sslConfig.caCert.empty()) {
                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificate loaded";
                        LOG(TRACE) << "  " << sslConfig.caCert;
                    } else {
                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificate not loaded from a file";
                    }
                    if (!sslConfig.caCertDir.empty()) {
                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates load from";
                        LOG(TRACE) << "  " << sslConfig.caCertDir;
                    } else {
                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates not loaded from a directory";
                    }
                }
            } else {
                LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificate not loaded from a file";
                LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates not loaded from a directory";
            }
            if (!sslErr && sslConfig.caCertUseDefaultDir) {
                if (SSL_CTX_set_default_verify_paths(ctx) == 0) {
                    ssl_log_error(sslConfig.instanceName + " SSL/TLS: CA certificates error load from default openssl CA directory");
                    sslErr = true;
                } else {
                    LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates enabled load from default openssl CA directory";
                }
            } else {
                LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates not loaded from default openssl CA directory";
            }
            if (!sslErr) {
                SSL_CTX_set_verify_depth(ctx, 5);
                SSL_CTX_set_verify(ctx,
                                   (sslConfig.caCertAcceptUnknown ? SSL_VERIFY_NONE
                                    : (!sslConfig.caCert.empty() || !sslConfig.caCertDir.empty() || sslConfig.caCertUseDefaultDir)
                                        ? SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT
                                        : 0),
                                   verify_callback);
                if ((SSL_CTX_get_verify_mode(ctx) & SSL_VERIFY_PEER) != 0) {
                    LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA requested verify";
                }
                if (!sslConfig.cert.empty()) {
                    if (SSL_CTX_use_certificate_chain_file(ctx, sslConfig.cert.c_str()) == 0) {
                        ssl_log_error(sslConfig.instanceName + " SSL/TLS: Cert chain error loading from file '" + sslConfig.cert + "'");
                        sslErr = true;
                    } else if (!sslConfig.certKey.empty()) {
                        if (!sslConfig.password.empty()) {
                            SSL_CTX_set_default_passwd_cb(ctx, password_callback);
                            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(sslConfig.password.c_str()));
                        }
                        if (SSL_CTX_use_PrivateKey_file(ctx, sslConfig.certKey.c_str(), SSL_FILETYPE_PEM) == 0) {
                            ssl_log_error(sslConfig.instanceName + " SSL/TLS: Cert chain key error loading file '" + sslConfig.certKey +
                                          "'");
                            sslErr = true;
                        } else if (SSL_CTX_check_private_key(ctx) != 1) {
                            ssl_log_error(sslConfig.instanceName + " SSL/TLS: Cert chain key error");

                            LOG(TRACE) << "  " << sslConfig.certKey;
                            sslErr = true;
                        } else {
                            LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: Cert chain key loaded";
                            LOG(TRACE) << "  " << sslConfig.certKey;

                            LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: Cert chain loaded";
                            LOG(TRACE) << "  " << sslConfig.cert;
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
            } else {
                SSL_CTX_free(ctx);
                ctx = nullptr;
            }
        }

        return ctx;
    }

    std::map<std::string, SSL_CTX*> ssl_get_sans(SSL_CTX* sslCtx) {
        std::map<std::string, SSL_CTX*> sans;

        if (sslCtx != nullptr) {
            X509* x509 = SSL_CTX_get0_certificate(sslCtx);
            if (x509 != nullptr) {
                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(x509, NID_subject_alt_name, nullptr, nullptr));
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
                for (int32_t i = 0; i < altNameCount; ++i) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
                    if (generalName->type == GEN_DNS || generalName->type == GEN_URI || generalName->type == GEN_EMAIL) {
                        const std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        sans.insert({subjectAltName, sslCtx});
                    }
                }
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
            }
        }

        return sans;
    }

    void ssl_set_sni(SSL* ssl, const std::string& sni) {
        if (!sni.empty()) {
            SSL_set_tlsext_host_name(ssl, sni.data());
        }
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

    // From: https://www.bit-hive.com/documents/openssl-tutorial/
    std::string ssl_get_servername_from_client_hello(SSL* ssl) {
        const unsigned char* ext = nullptr;
        size_t ext_len = 0;
        size_t p = 0;
        size_t server_name_list_len = 0;
        size_t server_name_len = 0;

        if (SSL_client_hello_get0_ext(ssl, TLSEXT_TYPE_server_name, &ext, &ext_len) == 0) {
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
        const utils::PreserveErrno preserveErrno;

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
            case SSL_ERROR_SSL:
                ssl_log_error(message);
                break;
            default:
                ssl_log_warning(message);
                break;
        }
    }

    void ssl_log_error(const std::string& message) {
        LOG(ERROR) << message;
        LOG(ERROR) << "  " << ERR_error_string(ERR_get_error(), nullptr);

        unsigned long errorCode = 0;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(ERROR) << "  " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_warning(const std::string& message) {
        LOG(WARNING) << message;
        LOG(WARNING) << "  " << ERR_error_string(ERR_get_error(), nullptr);

        unsigned long errorCode = 0;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(WARNING) << "  " << ERR_error_string(errorCode, nullptr);
        }
    }

    void ssl_log_info(const std::string& message) {
        LOG(INFO) << message;
        LOG(INFO) << "  " << ERR_error_string(ERR_get_error(), nullptr);

        unsigned long errorCode = 0;
        while ((errorCode = ERR_get_error()) != 0) {
            LOG(INFO) << "  " << ERR_error_string(errorCode, nullptr);
        }
    }

    bool match(const char* first, const char* second) {
        // If we reach at the end of both strings, we are done
        if (*first == '\0' && *second == '\0') {
            return true;
        }

        // Make sure to eliminate consecutive '*'
        if (*first == '*') {
            while (*(first + 1) == '*') {
                first++;
            }
        }

        // Make sure that the characters after '*' are present
        // in second string. This function assumes that the
        // first string will not contain two consecutive '*'
        if (*first == '*' && *(first + 1) != '\0' && *second == '\0') {
            return false;
        }

        // If the first string contains '?', or current
        // characters of both strings match
        if (*first == '?' || *first == *second) {
            return match(first + 1, second + 1);
        }

        // If there is *, then there are two possibilities
        // a) We consider current character of second string
        // b) We ignore current character of second string.
        if (*first == '*') {
            return match(first + 1, second) || match(first, second + 1);
        }

        return false;
    }

} // namespace core::socket::stream::tls
