/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef NET_IN_STREAM_SOCKETCLIENT_H
#define NET_IN_STREAM_SOCKETCLIENT_H

#include "core/socket/stream/SocketClient.h"        // IWYU pragma: export
#include "net/in/phy/stream/PhysicalSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    template <template <typename PhysicalSocketClient, typename ConfigSocketClientT> typename SocketConnectorT,
              typename ConfigSocketClientT,
              typename SocketContextFactoryT,
              typename... Args>
    class SocketClient
        : public core::socket::stream::SocketClient<SocketConnectorT<net::in::phy::stream::PhysicalSocketClient, ConfigSocketClientT>,
                                                    SocketContextFactoryT,
                                                    Args...> {
    private:
        using Super = core::socket::stream::
            SocketClient<SocketConnectorT<net::in::phy::stream::PhysicalSocketClient, ConfigSocketClientT>, SocketContextFactoryT, Args...>;

    public:
        using Super::Super;

        using Super::connect;

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Remote::setHost(ipOrHostname).setPort(port);

            connect(onStatus);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindHost,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Remote::setHost(ipOrHostname).setPort(port);
            Super::getConfig().Local::setHost(bindHost);

            connect(onStatus);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     uint16_t bindPort,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Remote::setHost(ipOrHostname).setPort(port);
            Super::getConfig().Local::setHost(bindPort);

            connect(onStatus);
        }

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindHost,
                     uint16_t bindPort,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Remote::setHost(ipOrHostname).setPort(port);
            Super::getConfig().Local::setHost(bindHost).setPort(bindPort);

            connect(onStatus);
        }
    };

} // namespace net::in::stream

#endif // NET_IN_STREAM_SOCKETCLIENT_H
