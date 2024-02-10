/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

namespace core::socket::stream::tls {
    template <typename PhysicalSocketT>
    class SocketConnection;
}

// IWYU pragma: no_include "core/socket/stream/legacy/SocketConnection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

using SSL_CTX = struct ssl_ctx_st;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename PhysicalSocketServerT, typename ConfigT>
    class SocketAcceptor
        : private core::socket::stream::SocketAcceptor<PhysicalSocketServerT, ConfigT, core::socket::stream::tls::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<PhysicalSocketServerT, ConfigT, core::socket::stream::tls::SocketConnection>;

    public:
        using SocketAddress = typename Super::SocketAddress;
        using Config = typename Super::Config;
        using SocketConnection = typename Super::SocketConnection;

        using Super::config;

        SocketAcceptor(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                       const std::shared_ptr<Config>& config);

        SocketAcceptor(const SocketAcceptor& socketAcceptor);

        ~SocketAcceptor() override;

    private:
        void useNextSocketAddress() override;

        void init() final;

        SSL_CTX* masterSslCtx = nullptr;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
