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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTOR_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTOR_H

#include "core/socket/stream/SocketConnector.h"         // IWYU pragma: export
#include "core/socket/stream/legacy/SocketConnection.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename SocketClientT>
    class SocketConnector : protected core::socket::stream::SocketConnector<SocketClientT, core::socket::stream::legacy::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketConnector<SocketClientT, core::socket::stream::legacy::SocketConnection>;
        using SocketClient = SocketClientT;
        using SocketAddress = typename Super::SocketAddress;
        using Config = typename Super::Config;

    public:
        using SocketConnection = typename Super::SocketConnection;

        SocketConnector(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::map<std::string, std::any>& options,
                        const std::shared_ptr<Config>& config)
            : Super(
                  socketContextFactory,
                  onConnect,
                  [onConnected](SocketConnection* socketConnection) -> void {
                      onConnected(socketConnection);
                      socketConnection->onConnected();
                  },
                  onDisconnect,
                  options,
                  config) {
        }

        using Super::connect;
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTOR_H
