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

#include "core/socket/stream/SocketServer.h" // IWYU pragma: export
#include "core/socket/stream/tls/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/x509.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketServerT, typename SocketContextFactoryT>
    class SocketServer
        : public core::socket::stream::SocketServer<SocketServerT, core::socket::stream::tls::SocketAcceptor, SocketContextFactoryT> {
    private:
        using Super = core::socket::stream::SocketServer<SocketServerT, core::socket::stream::tls::SocketAcceptor, SocketContextFactoryT>;
        using Super::Super;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        SocketServer(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : Super(name, onConnect, onConnected, onDisconnect, options)
            , sniSslCtxs(new std::map<std::string, SSL_CTX*>, [](std::map<std::string, SSL_CTX*>* sniSslCtxs) {
                for (const auto& [domain, sniSslCtx] : *sniSslCtxs) {
                    ssl_ctx_free(sniSslCtx);
                }
                delete sniSslCtxs;
            }) {
            Super::_options.insert({{"SNI_SSL_CTXS", sniSslCtxs}});
        }

        SocketServer(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : SocketServer("", onConnect, onConnected, onDisconnect, options) {
        }

    public:
        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& sniCert) {
            SSL_CTX* sslCtx = ssl_ctx_new(sniCert, true);

            if (sslCtx != nullptr) {
                sniSslCtxs->insert_or_assign(domain, sslCtx);
                VLOG(2) << "SSL_CTX for domain '" << domain << "' installed";
            } else {
                VLOG(2) << "Can not create SSL_CTX for SNI '" << domain << "'";
            }
        }

        void addSniCerts(const std::map<std::string, std::map<std::string, std::any>>& sniCerts) {
            for (const auto& [domain, cert] : sniCerts) {
                addSniCert(domain, cert);
            }
        }

        void forceSni(bool forceSni = true) {
            Super::_options.insert_or_assign("FORCE_SNI", forceSni);
        }

        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
