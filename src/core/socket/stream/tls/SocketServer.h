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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H

#include "core/socket/stream/SocketServer.h"       // IWYU pragma: export
#include "core/socket/stream/tls/SocketAcceptor.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename ServerSocketT, typename ServerConfigT, typename SocketContextFactoryT>
    class SocketServer
        : public core::socket::stream::SocketServer<
              ServerSocketT,
              core::socket::stream::tls::SocketAcceptor<ServerConfigT, typename ServerSocketT::Socket>,
              SocketContextFactoryT> {
    private:
        using Super =
            core::socket::stream::SocketServer<ServerSocketT,
                                               core::socket::stream::tls::SocketAcceptor<ServerConfigT, typename ServerSocketT::Socket>,
                                               SocketContextFactoryT>;
        using Super::Super;

    public:
        using Socket = typename Super::Socket;
        using SocketAddress = typename Super::SocketAddress;
        using SocketConnection = typename Super::SocketConnection;

        SocketServer(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : Super(name, onConnect, onConnected, onDisconnect, options)
            , sniSslCtxs(new std::map<std::string, SSL_CTX*>, [this](std::map<std::string, SSL_CTX*>* sniSslCtxs) {
                freeSniCerts(sniSslCtxs);
            }) {
            this->options.insert({{"SNI_SSL_CTXS", sniSslCtxs}});
            this->options.insert({{"FORCE_SNI", &forceSni}});
        }

    private:
        void addSniCtx(const std::string& domain, SSL_CTX* sslCtx) {
            if (sslCtx != nullptr) {
                sniSslCtxs->insert({{domain, sslCtx}});
                VLOG(2) << "SSL_CTX for domain '" << domain << "' installed";
            } else {
                VLOG(2) << "Can not create SSL_CTX for SNI '" << domain << "'";
            }
        }

    public:
        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& sniCert) {
            SSL_CTX* sslCtx = ssl_ctx_new(sniCert, true);
            addSniCtx(domain, sslCtx);
        }

        void addSniCerts(const std::map<std::string, std::map<std::string, std::any>>& sniCerts) {
            for (const auto& [domain, cert] : sniCerts) {
                addSniCert(domain, cert);
            }
        }

        void setForceSni() {
            forceSni = true;
        }

    private:
        void freeSniCerts(std::map<std::string, SSL_CTX*>* sniSslCtxs) {
            for (const auto& [domain, sniSslCtx] : *sniSslCtxs) { // cppcheck-suppress unusedVariable
                ssl_ctx_free(sniSslCtx);
            }
            delete sniSslCtxs;
        }

        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
        bool forceSni = false;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
