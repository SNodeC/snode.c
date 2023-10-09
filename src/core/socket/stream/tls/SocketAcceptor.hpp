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

#include "core/socket/stream/SocketAcceptor.hpp" // IWYU pragma: export
#include "core/socket/stream/tls/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <algorithm>
#include <openssl/ssl.h>    // IWYU pragma: export
#include <openssl/x509v3.h> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename PhysicalServerSocket, typename Config>
    core::socket::stream::tls::SocketAcceptor<PhysicalServerSocket, Config>::SocketAcceptor(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : Super(
              socketContextFactory,
              [onConnect, this](SocketConnection* socketConnection) -> void { // onConnect
                  socketConnection->startSSL(this->masterSslCtx, Super::config->getInitTimeout(), Super::config->getShutdownTimeout());

                  onConnect(socketConnection);
              },
              [socketContextFactory, onConnected, this](SocketConnection* socketConnection) -> void { // on Connected
                  SSL* ssl = socketConnection->getSSL();

                  if (ssl != nullptr) {
                      SSL_set_accept_state(ssl);

                      socketConnection->doSSLHandshake(
                          [socketContextFactory, onConnected, socketConnection, instanceName = this->config->getInstanceName()]()
                              -> void { // onSuccess
                              LOG(TRACE) << instanceName << ": SSL/TLS initial handshake success";

                              onConnected(socketConnection);
                              socketConnection->connected(socketContextFactory);
                          },
                          [socketConnection, instanceName = this->config->getInstanceName()]() -> void { // onTimeout
                              LOG(TRACE) << instanceName << ": SSL/TLS initial handshake timed out";

                              socketConnection->close();
                          },
                          [socketConnection, instanceName = this->config->getInstanceName()](int sslErr) -> void { // onStatus
                              ssl_log(instanceName + ": SSL/TLS initial handshake failed", sslErr);

                              socketConnection->close();
                          });
                  } else {
                      ssl_log_error(this->config->getInstanceName() + ": SSL/TLS initialization failed");

                      socketConnection->close();
                  }
              },
              [onDisconnect, instanceName = config->getInstanceName()](SocketConnection* socketConnection) -> void { // onDisconnect
                  LOG(TRACE) << instanceName << ": SSL/TLS connection destroyed";

                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onStatus,
              config) {
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketAcceptor<PhysicalSocketServer, Config>::~SocketAcceptor() {
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

    template <typename PhysicalSocketServer, typename Config>
    void SocketAcceptor<PhysicalSocketServer, Config>::initAcceptEvent() {
        if (!config->getDisabled()) {
            masterSslCtx = ssl_ctx_new(config);

            if (masterSslCtx != nullptr) {
                masterSslCtxSans = ssl_get_sans(masterSslCtx);

                for (const std::string& san : masterSslCtxSans) {
                    LOG(TRACE) << config->getInstanceName() << ": SSL_CTX (M) for (san)'" << san << "' as master installed";
                }

                for (const auto& [domain, sniCertConf] : config->getSniCerts()) {
                    if (!sniCertConf.empty()) {
                        SSL_CTX* sniSslCtx = ssl_ctx_new(sniCertConf);

                        if (sniSslCtx != nullptr) {
                            if (!domain.empty()) {
                                if (sniSslCtxs.contains(domain)) {
                                    ssl_ctx_free(sniSslCtxs[domain]);
                                }
                                sniSslCtxs.insert_or_assign(domain, sniSslCtx);

                                LOG(TRACE) << config->getInstanceName() << ": SSL_CTX (E) sni for (dom)'" << domain
                                           << "' as server name indication (sni) installed";
                            }

                            for (const std::string& san : ssl_get_sans(sniSslCtx)) {
                                sniSslCtxs.insert({san, sniSslCtx});

                                LOG(TRACE) << config->getInstanceName() << ": SSL_CTX (S) sni for (san)'" << san
                                           << "' as server name indication (sni) installed";
                            }
                        } else {
                            LOG(TRACE) << config->getInstanceName() << ": Can not create SNI_SSL_CTX for domain '" << domain << "'";
                        }
                    }
                }
                forceSni = config->getForceSni();

                SSL_CTX_set_client_hello_cb(masterSslCtx, clientHelloCallback, this);

                Super::initAcceptEvent();
            } else {
                LOG(ERROR) << config->getInstanceName() << ": Master SSL/TLS certificate activation failed";

                Super::onStatus(Super::config->Local::getSocketAddress(), core::socket::State::ERROR);
                Super::destruct();
            }
        } else {
            Super::initAcceptEvent();
        }
    }

    template <typename PhysicalSocketServer, typename Config>
    SSL_CTX* SocketAcceptor<PhysicalSocketServer, Config>::getMasterSniCtx(const std::string& serverNameIndication) {
        SSL_CTX* sniSslCtx = nullptr;

        LOG(TRACE) << config->getInstanceName() << ": Search for sni='" << serverNameIndication << "' in master certificate";

        std::set<std::string>::iterator masterSniIt =
            std::find_if(masterSslCtxSans.begin(),
                         masterSslCtxSans.end(),
                         [&serverNameIndication, config = this->config](const std::string& sni) -> bool {
                             LOG(TRACE) << config->getInstanceName() << ":  .. " << sni.c_str();
                             return match(sni.c_str(), serverNameIndication.c_str());
                         });
        if (masterSniIt != masterSslCtxSans.end()) {
            LOG(TRACE) << config->getInstanceName() << ": found '" << *masterSniIt << "'";
            sniSslCtx = masterSslCtx;
        } else {
            LOG(TRACE) << config->getInstanceName() << ": not found";
        }

        return sniSslCtx;
    }

    template <typename PhysicalSocketServer, typename Config>
    SSL_CTX* SocketAcceptor<PhysicalSocketServer, Config>::getPoolSniCtx(const std::string& serverNameIndication) {
        SSL_CTX* sniCtx = nullptr;

        LOG(TRACE) << config->getInstanceName() << ": Search for sni='" << serverNameIndication << "' in sni certificates";

        std::map<std::string, SSL_CTX*>::iterator sniPairIt =
            std::find_if(sniSslCtxs.begin(),
                         sniSslCtxs.end(),
                         [&serverNameIndication, config = this->config](const std::pair<std::string, SSL_CTX*>& sniPair) -> bool {
                             LOG(TRACE) << config->getInstanceName() << ":  .. " << sniPair.first.c_str();
                             return match(sniPair.first.c_str(), serverNameIndication.c_str());
                         });

        if (sniPairIt != sniSslCtxs.end()) {
            LOG(TRACE) << config->getInstanceName() << ": found '" << sniPairIt->first << "'";
            sniCtx = sniPairIt->second;
        } else {
            LOG(TRACE) << config->getInstanceName() << ": not found";
        }

        return sniCtx;
    }

    template <typename PhysicalSocketServer, typename Config>
    SSL_CTX* SocketAcceptor<PhysicalSocketServer, Config>::getSniCtx(const std::string& serverNameIndication) {
        SSL_CTX* sniSslCtx = getMasterSniCtx(serverNameIndication);

        if (sniSslCtx == nullptr) {
            sniSslCtx = getPoolSniCtx(serverNameIndication);
        }

        return sniSslCtx;
    }

    template <typename PhysicalSocketServer, typename Config>
    int SocketAcceptor<PhysicalSocketServer, Config>::clientHelloCallback(SSL* ssl, int* al, void* arg) {
        int ret = SSL_CLIENT_HELLO_SUCCESS;

        SocketAcceptor* socketAcceptor = static_cast<SocketAcceptor*>(arg);

        std::string serverNameIndication = ssl_get_servername_from_client_hello(ssl);

        if (!serverNameIndication.empty()) {
            SSL_CTX* sniSslCtx = socketAcceptor->getSniCtx(serverNameIndication);
            if (sniSslCtx != nullptr) {
                LOG(TRACE) << socketAcceptor->config->getInstanceName() << ": Setting sni certificate for '" << serverNameIndication << "'";
                ssl_set_ssl_ctx(ssl, sniSslCtx);
            } else if (socketAcceptor->forceSni) {
                LOG(WARNING) << socketAcceptor->config->getInstanceName() << ": No sni certificate found for '" << serverNameIndication
                             << "' but forceSni set - terminating";
                ret = SSL_CLIENT_HELLO_ERROR;
                *al = SSL_AD_UNRECOGNIZED_NAME;
            } else {
                LOG(TRACE) << socketAcceptor->config->getInstanceName() << ": No sni certificate found for '" << serverNameIndication
                           << "' still using master certificate";
            }
        } else {
            LOG(TRACE) << socketAcceptor->config->getInstanceName() << ": No sni certificate set - the client did not request one";
        }

        return ret;
    }

    template <typename PhysicalSocketServer, typename Config>
    bool SocketAcceptor<PhysicalSocketServer, Config>::match(const char* first, const char* second) {
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

} // namespace core::socket::stream::tls
