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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H

#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/tls/SocketConnection.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <map>
#include <set>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketServerT>
    class SocketAcceptor : protected core::socket::stream::SocketAcceptor<SocketServerT, core::socket::stream::tls::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<SocketServerT, core::socket::stream::tls::SocketConnection>;
        using SocketServer = SocketServerT;
        using SocketAddress = typename Super::SocketAddress;
        using Config = typename Super::Config;

    public:
        using SocketConnection = typename Super::SocketConnection;

        using Super::config;

        SocketAcceptor(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, int)>& onError,
                       const std::shared_ptr<Config>& config)
            : Super(
                  socketContextFactory,
                  [onConnect, this](SocketConnection* socketConnection) -> void { // onConnect
                      socketConnection->startSSL(this->masterSslCtx, this->config->getInitTimeout(), this->config->getShutdownTimeout());

                      onConnect(socketConnection);
                  },
                  [onConnected](SocketConnection* socketConnection) -> void {
                      SSL* ssl = socketConnection->getSSL();

                      if (ssl != nullptr) {
                          SSL_set_accept_state(ssl);

                          socketConnection->doSSLHandshake(
                              [&onConnected, socketConnection]() -> void { // onSuccess
                                  LOG(INFO) << "SSL/TLS initial handshake success";
                                  onConnected(socketConnection);
                                  socketConnection->onConnected();
                              },
                              []() -> void { // onTimeout
                                  LOG(WARNING) << "SSL/TLS initial handshake timed out";
                              },
                              [](int sslErr) -> void { // onError
                                  ssl_log("SSL/TLS initial handshake failed", sslErr);
                              });
                      } else {
                          socketConnection->close();
                          ssl_log_error("SSL/TLS initialization failed");
                      }
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  onError,
                  config) {
        }

        ~SocketAcceptor() override {
            if (masterSslCtx != nullptr) {
                ssl_ctx_free(masterSslCtx);
            }

            for (const auto& [domain, ctx] : sniSslCtxs) {
                if (ctx != nullptr) {
                    ssl_ctx_free(ctx);

                    std::for_each(sniSslCtxs.begin(), sniSslCtxs.end(), [sslCtx = ctx](auto& sniSslCtxsItem) -> void {
                        if (sniSslCtxsItem.second == sslCtx) {
                            sniSslCtxsItem.second = nullptr;
                        }
                    });
                }
            }
        }

    private:
        void initAcceptEvent() override {
            if (!config->getDisabled()) {
                masterSslCtx = ssl_ctx_new(config);

                if (masterSslCtx != nullptr) {
                    masterSslCtxSans = ssl_get_sans(masterSslCtx);

                    for (const std::string& san : masterSslCtxSans) {
                        LOG(INFO) << "SSL_CTX for (san)'" << san << "' as master installed";
                    }

                    SSL_CTX_set_client_hello_cb(masterSslCtx, clientHelloCallback, this);
                    forceSni = config->getForceSni();

                    for (const auto& [domain, sniCertConf] : config->getSniCerts()) {
                        if (!sniCertConf.empty()) {
                            SSL_CTX* sniSslCtx = ssl_ctx_new(sniCertConf);

                            if (sniSslCtx != nullptr) {
                                if (!domain.empty()) {
                                    if (sniSslCtxs.contains(domain)) {
                                        ssl_ctx_free(sniSslCtxs[domain]);
                                    }
                                    sniSslCtxs.insert_or_assign(domain, sniSslCtx);

                                    LOG(INFO) << "SSL_CTX for (dom)'" << domain << "' as server name indication (sni) installed";
                                }

                                for (const std::string& san : ssl_get_sans(sniSslCtx)) {
                                    sniSslCtxs.insert({san, sniSslCtx});

                                    LOG(INFO) << "SSL_CTX for (san)'" << san << "' as server name indication (sni) installed";
                                }
                            } else {
                                LOG(INFO) << "Can not create SSL_CTX for domain '" << domain << "'";
                            }
                        }
                    }

                    Super::initAcceptEvent();
                } else {
                    Super::onError(Super::config->getLocalAddress(), EINVAL);
                    Super::destruct();
                }
            } else {
                Super::initAcceptEvent();
            }
        }

        SSL_CTX* getMasterSniCtx(const std::string& serverNameIndication) {
            SSL_CTX* sniSslCtx = nullptr;

            LOG(INFO) << "Search for sni = '" << serverNameIndication << "' in master certificate";

            std::set<std::string>::iterator masterSniIt =
                std::find_if(masterSslCtxSans.begin(), masterSslCtxSans.end(), [&serverNameIndication](const std::string& sni) -> bool {
                    LOG(TRACE) << "  .. " << sni.c_str();
                    return match(sni.c_str(), serverNameIndication.c_str());
                });
            if (masterSniIt != masterSslCtxSans.end()) {
                LOG(INFO) << "found: " << *masterSniIt;
                sniSslCtx = masterSslCtx;
            } else {
                LOG(INFO) << "not found";
            }

            return sniSslCtx;
        }

        SSL_CTX* getPoolSniCtx(const std::string& serverNameIndication) {
            SSL_CTX* sniCtx = nullptr;

            LOG(INFO) << "Search for sni = '" << serverNameIndication << "' in sni certificates";

            std::map<std::string, SSL_CTX*>::iterator sniPairIt = std::find_if(
                sniSslCtxs.begin(), sniSslCtxs.end(), [&serverNameIndication](const std::pair<std::string, SSL_CTX*>& sniPair) -> bool {
                    LOG(TRACE) << "  .. " << sniPair.first.c_str();
                    return match(sniPair.first.c_str(), serverNameIndication.c_str());
                });

            if (sniPairIt != sniSslCtxs.end()) {
                LOG(INFO) << "found: " << sniPairIt->first;
                sniCtx = sniPairIt->second;
            } else {
                LOG(INFO) << "not found";
            }

            return sniCtx;
        }

        SSL_CTX* getSniCtx(const std::string& serverNameIndication) {
            SSL_CTX* sniSslCtx = getMasterSniCtx(serverNameIndication);

            if (sniSslCtx == nullptr) {
                sniSslCtx = getPoolSniCtx(serverNameIndication);
            }

            return sniSslCtx;
        }

        static int clientHelloCallback(SSL* ssl, int* al, void* arg) {
            int ret = SSL_CLIENT_HELLO_SUCCESS;

            SocketAcceptor* socketAcceptor = static_cast<SocketAcceptor*>(arg);

            std::string serverNameIndication = ssl_get_servername_from_client_hello(ssl);

            if (!serverNameIndication.empty()) {
                SSL_CTX* sniSslCtx = socketAcceptor->getSniCtx(serverNameIndication);
                if (sniSslCtx != nullptr) {
                    LOG(INFO) << "Setting sni certificate for " << serverNameIndication;
                    ssl_set_ssl_ctx(ssl, sniSslCtx);
                } else if (socketAcceptor->forceSni) {
                    LOG(WARNING) << "No sni certificate found but forceSni set - terminating";
                    ret = SSL_CLIENT_HELLO_ERROR;
                    *al = SSL_AD_UNRECOGNIZED_NAME;
                } else {
                    LOG(INFO) << "No sni certificate found - still using master certificate";
                }
            } else {
                LOG(INFO) << "No sni certificate set - the client did not request one";
            }

            return ret;
        }

        // From https://www.geeksforgeeks.org/wildcard-character-matching/
        //
        // The main function that checks if two given strings
        // match. The first string may contain wildcard characters
        static bool match(const char* first, const char* second) {
            // If we reach at the end of both strings, we are done
            if (*first == '\0' && *second == '\0')
                return true;

            // Make sure to eliminate consecutive '*'
            if (*first == '*') {
                while (*(first + 1) == '*')
                    first++;
            }

            // Make sure that the characters after '*' are present
            // in second string. This function assumes that the
            // first string will not contain two consecutive '*'
            if (*first == '*' && *(first + 1) != '\0' && *second == '\0')
                return false;

            // If the first string contains '?', or current
            // characters of both strings match
            if (*first == '?' || *first == *second)
                return match(first + 1, second + 1);

            // If there is *, then there are two possibilities
            // a) We consider current character of second string
            // b) We ignore current character of second string.
            if (*first == '*')
                return match(first + 1, second) || match(first, second + 1);
            return false;
        }

        SSL_CTX* masterSslCtx = nullptr;
        std::set<std::string> masterSslCtxSans;

        std::map<std::string, SSL_CTX*> sniSslCtxs;
        bool forceSni = false;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
