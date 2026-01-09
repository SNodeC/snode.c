/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "net/config/stream/tls/ConfigSocketServer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <list>
#include <openssl/ssl.h>
#include <variant>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config::stream::tls {

    template <typename ConfigSocketServerBase>
    ConfigSocketServer<ConfigSocketServerBase>::ConfigSocketServer(const std::string& name)
        : net::config::ConfigInstance(name, net::config::ConfigInstance::Role::SERVER)
        , ConfigSocketServerBase(this)
        , net::config::ConfigTlsServer(this) {
    }

    template <typename ConfigSocketServerBase>
    ConfigSocketServer<ConfigSocketServerBase>::~ConfigSocketServer() {
        if (sslCtx != nullptr) {
            core::socket::stream::tls::ssl_ctx_free(sslCtx);
        }

        for (SSL_CTX* sniCtx : sniCtxs) {
            if (sniCtx != nullptr) {
                core::socket::stream::tls::ssl_ctx_free(sniCtx);
            }
        }
    }

    template <typename ConfigSocketServerBase>
    SSL_CTX* ConfigSocketServer<ConfigSocketServerBase>::getSslCtx() {
        if (sslCtx == nullptr) {
            core::socket::stream::tls::SslConfig sslConfig(true);

            sslConfig.instanceName = getInstanceName();

            sslConfig.cert = getCert();
            sslConfig.certKey = getCertKey();
            sslConfig.password = getCertKeyPassword();
            sslConfig.caCert = getCaCert();
            sslConfig.caCertDir = getCaCertDir();
            sslConfig.cipherList = getCipherList();
            sslConfig.sslOptions = getSslOptions();
            sslConfig.caCertUseDefaultDir = getCaCertUseDefaultDir();
            sslConfig.caCertAcceptUnknown = getCaCertAcceptUnknown();

            sslCtx = core::socket::stream::tls::ssl_ctx_new(sslConfig);
        }

        if (sniCtxMap.empty()) {
            std::map<std::string, SSL_CTX*> sslSans = core::socket::stream::tls::ssl_get_sans(sslCtx);

            sniCtxMap.insert(sslSans.begin(), sslSans.end());

            for (const auto& [sni, ctx] : sniCtxMap) {
                LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (M) sni for '" << sni << "' from master certificate installed";
            }

            for (const auto& [domain, sniCertConf] : getSniCerts()) {
                if (!domain.empty()) {
                    core::socket::stream::tls::SslConfig sslConfig(true);

                    sslConfig.instanceName = getInstanceName();

                    for (const auto& [key, value] : sniCertConf) {
                        if (key == "Cert") {
                            sslConfig.cert = std::get<std::string>(value);
                        } else if (key == "CertKey") {
                            sslConfig.certKey = std::get<std::string>(value);
                        } else if (key == "CertKeyPassword") {
                            sslConfig.password = std::get<std::string>(value);
                        } else if (key == "CaCert") {
                            sslConfig.caCert = std::get<std::string>(value);
                        } else if (key == "CaCertDir") {
                            sslConfig.caCertDir = std::get<std::string>(value);
                        } else if (key == "CaCertUseDefaultDir") {
                            sslConfig.caCertUseDefaultDir = std::get<bool>(value);
                        } else if (key == "CaCertAcceptUnknown") {
                            sslConfig.caCertAcceptUnknown = std::get<bool>(value);
                        } else if (key == "CipherList") {
                            sslConfig.cipherList = std::get<std::string>(value);
                        } else if (key == "SslOptions") {
                            sslConfig.sslOptions = std::get<ssl_option_t>(value);
                        }
                    }

                    SSL_CTX* newCtx = core::socket::stream::tls::ssl_ctx_new(sslConfig);

                    if (newCtx != nullptr) {
                        sniCtxs.push_back(newCtx);
                        sniCtxMap.insert_or_assign(domain, newCtx);

                        LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (E) sni for '" << domain << "' explicitly installed";

                        for (const auto& [san, ctx] : core::socket::stream::tls::ssl_get_sans(newCtx)) {
                            sniCtxMap.insert_or_assign(san, ctx);

                            LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (S) sni for '" << san << "' from SAN installed";
                        }
                    } else {
                        LOG(WARNING) << getInstanceName() << " SSL/TLS: Can not create SNI_SSL_CTX for domain '" << domain << "'";
                    }
                }
            }
            LOG(TRACE) << getInstanceName() << " SSL/TLS: SNI list result:";
            for (const auto& [sni, ctx] : sniCtxMap) {
                LOG(TRACE) << "  " << sni;
            }
        }

        return sslCtx;
    }

    template <typename ConfigSocketServerBase>
    SSL_CTX* ConfigSocketServer<ConfigSocketServerBase>::getSniCtx(const std::string& serverNameIndication) {
        LOG(TRACE) << getInstanceName() << " SSL/TLS SNI: Lookup for sni='" << serverNameIndication << "' in sni certificates";

        SSL_CTX* sniCtx = nullptr;

        std::map<std::string, SSL_CTX*>::iterator sniPairIt = std::find_if(
            sniCtxMap.begin(), sniCtxMap.end(), [&serverNameIndication, this](const std::pair<std::string, SSL_CTX*>& sniPair) -> bool {
                LOG(TRACE) << getInstanceName() << " SSL/TLS SNI:  .. " << sniPair.first.c_str();
                return core::socket::stream::tls::match(sniPair.first.c_str(), serverNameIndication.c_str());
            });

        if (sniPairIt != sniCtxMap.end()) {
            LOG(TRACE) << getInstanceName() << " SSL/TLS SNI: found for " << serverNameIndication << " -> '" << sniPairIt->first << "'";
            sniCtx = sniPairIt->second;
        } else {
            LOG(WARNING) << getInstanceName() << " SSL/TL SNI: not found for " << serverNameIndication;
        }

        return sniCtx;
    }

} // namespace net::config::stream::tls
