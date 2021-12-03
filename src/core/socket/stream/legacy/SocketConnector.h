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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTOR_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTOR_H

#include "core/socket/stream/SocketConnector.h"
#include "core/socket/stream/legacy/SocketConnection.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename SocketT>
    using SocketConnectorSuper = core::socket::stream::SocketConnector<core::socket::stream::legacy::SocketConnection<SocketT>>;

    template <typename SocketT>
    class SocketConnector : public SocketConnectorSuper<SocketT> {
    private:
        using Super = SocketConnectorSuper<SocketT>;

        using Socket = typename Super::Socket;
        using SocketContextFactory = typename Super::SocketContextFactory;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        SocketConnector(const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                        const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::map<std::string, std::any>& options)
            : Super(
                  socketContextFactory,
                  onConnect,
                  [onConnected](SocketConnection* socketConnection) -> void {
                      socketConnection->SocketConnection::SocketReader::resume();
                      onConnected(socketConnection);
                  },
                  onDisconnect,
                  options) {
        }
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTOR_H
