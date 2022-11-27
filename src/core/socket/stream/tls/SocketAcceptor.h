/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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
#include "core/socket/stream/tls/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <map>
#include <memory>
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

        SocketAcceptor(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : Super(
                  socketContextFactory,
                  [onConnect, this](SocketConnection* socketConnection) -> void { // onConnect
                      socketConnection->startSSL(this->masterSslCtx, this->config->getInitTimeout(), this->config->getShutdownTimeout());

                      onConnect(socketConnection);
                  },
                  [onConnected, this](SocketConnection* socketConnection) -> void {
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
                  options)
            , sniSslCtxs(std::any_cast<std::shared_ptr<std::map<std::string, SSL_CTX*>>>(options.find("SNI_SSL_CTXS")->second)) {
        }

        ~SocketAcceptor() override {
            if (masterSslCtx != nullptr) {
                ssl_ctx_free(masterSslCtx);
            }
        }

        void listen(const std::shared_ptr<Config>& config, const std::function<void(const SocketAddress&, int)>& onError) {
            masterSslCtx = ssl_ctx_new(config, true);

            if (masterSslCtx == nullptr) {
                onError(config->getLocalAddress(), EINVAL);
                Super::destruct();
            } else {
                masterSslCtxDomains = ssl_get_sans(masterSslCtx);
                SSL_CTX_set_client_hello_cb(masterSslCtx, clientHelloCallback, this);
                forceSni = config->getForceSni();

                Super::listen(config, onError);
            }
        }

    private:
        SSL_CTX* getMasterSniCtx(const std::string& serverNameIndication) {
            SSL_CTX* sniSslCtx = nullptr;

            LOG(INFO) << "Search for sni = '" << serverNameIndication << "' in master certificate";

            std::set<std::string>::iterator masterSniIt = std::find_if(
                masterSslCtxDomains.begin(), masterSslCtxDomains.end(), [&serverNameIndication](const std::string& sni) -> bool {
                    LOG(TRACE) << "  .. " << sni.c_str();
                    return match(sni.c_str(), serverNameIndication.c_str());
                });
            if (masterSniIt != masterSslCtxDomains.end()) {
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
                sniSslCtxs->begin(), sniSslCtxs->end(), [&serverNameIndication](const std::pair<std::string, SSL_CTX*>& sniPair) -> bool {
                    LOG(TRACE) << "  .. " << sniPair.first.c_str();
                    return match(sniPair.first.c_str(), serverNameIndication.c_str());
                });

            if (sniPairIt != sniSslCtxs->end()) {
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
        std::set<std::string> masterSslCtxDomains;

        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
        bool forceSni = false;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
