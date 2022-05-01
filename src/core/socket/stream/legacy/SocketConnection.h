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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/legacy/SocketReader.h"
#include "core/socket/stream/legacy/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename SocketT>
    class SocketConnection
        : public core::socket::stream::
              SocketConnection<SocketT, core::socket::stream::legacy::SocketReader, core::socket::stream::legacy::SocketWriter> {
    private:
        using Super = core::socket::stream::
            SocketConnection<SocketT, core::socket::stream::legacy::SocketReader, core::socket::stream::legacy::SocketWriter>;

    private:
        using Socket = SocketT;
        using SocketReader = typename Super::SocketReader;
        using SocketWriter = typename Super::SocketWriter;

    public:
        using SocketAddress = typename Super::SocketAddress;

        SocketConnection(int fd,
                         const std::shared_ptr<core::socket::SocketContextFactory>& socketProtocolFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(SocketConnection*)>& onConnect,
                         const std::function<void(SocketConnection*)>& onDisconnect,
                         const utils::Timeval& readTimeout,
                         const utils::Timeval& writeTimeout,
                         std::size_t readBlockSize,
                         std::size_t writeBlockSize,
                         const utils::Timeval& terminateTimeout)
            : Super::Descriptor(fd)
            , Super(
                  socketProtocolFactory,
                  localAddress,
                  remoteAddress,
                  [onConnect, this]() -> void {
                      onConnect(this);
                  },
                  [onDisconnect, this]() -> void {
                      onDisconnect(this);
                  },
                  readTimeout,
                  writeTimeout,
                  readBlockSize,
                  writeBlockSize,
                  terminateTimeout) {
        }

    private:
        ~SocketConnection() override = default;

        template <typename ServerSocket>
        friend class SocketAcceptor;

        template <typename ClientSocket>
        friend class SocketConnector;
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETCONNECTION_H
