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

#ifndef NET_SOCKET_STREAM_TLS_SOCKETSERVER_H
#define NET_SOCKET_STREAM_TLS_SOCKETSERVER_H

#include "net/socket/stream/SocketServer.h"       // IWYU pragma: export
#include "net/socket/stream/tls/SocketAcceptor.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <map>
#include <memory>
#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream::tls {

    template <typename ServerSocketT, typename SocketContextFactoryT>
    class SocketServer
        : public net::socket::stream::SocketServer<ServerSocketT, SocketAcceptor<typename ServerSocketT::Socket>, SocketContextFactoryT> {
        using SocketServerBase =
            net::socket::stream::SocketServer<ServerSocketT, SocketAcceptor<typename ServerSocketT::Socket>, SocketContextFactoryT>;

        using SocketServerBase::SocketServerBase;

    public:
        using SocketConnection = typename SocketAcceptor<typename ServerSocketT::Socket>::SocketConnection;
        using SocketAddress = typename SocketConnection::Socket::SocketAddress;

        SocketServer(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : SocketServerBase(onConnect, onConnected, onDisconnect, options)
            , sniSslCtxs(new std::map<std::string, SSL_CTX*>, [](std::map<std::string, SSL_CTX*>* sniSslCtxs) {
                for (const auto& [domain, sniSslCtx] : *sniSslCtxs) {
                    ssl_ctx_free(sniSslCtx);
                }
                delete sniSslCtxs;
            }) {
            this->options.insert({{"SNI_SSL_CTXS", sniSslCtxs}});
        }

        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& options) {
            SSL_CTX* sSlCtx = ssl_ctx_new(options, true);

            if (sSlCtx != nullptr) {
                sniSslCtxs->insert({{domain, sSlCtx}});
                VLOG(2) << "SSL_CTX for domain '" << domain << "' installed";
            } else {
                VLOG(2) << "Can not create SSL_CTX for SNI '" << domain << "'";
            }
        }

    private:
        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
    };

} // namespace net::socket::stream::tls

#endif // NET_SOCKET_STREAM_TLS_SOCKETSERVER_H
