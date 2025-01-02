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

#ifndef NET_RC_STREAM_SOCKETSERVER_H
#define NET_RC_STREAM_SOCKETSERVER_H

#include "core/socket/stream/SocketServer.h"        // IWYU pragma: export
#include "net/rc/phy/stream/PhysicalSocketServer.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::stream {

    template <template <typename PhysicalSocketServer, typename ConfigSocketServer> typename SocketAcceptorT,
              typename ConfigSocketServerT,
              typename SocketContextFactoryT,
              typename... Args>
    class SocketServer
        : public core::socket::stream::SocketServer<SocketAcceptorT<net::rc::phy::stream::PhysicalSocketServer, ConfigSocketServerT>,
                                                    SocketContextFactoryT,
                                                    Args...> {
    private:
        using Super = core::socket::stream::
            SocketServer<SocketAcceptorT<net::rc::phy::stream::PhysicalSocketServer, ConfigSocketServerT>, SocketContextFactoryT, Args...>;

    public:
        using Super::Super;

        using Super::listen;

        void listen(uint8_t channel, const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Local::setChannel(channel);

            listen(onStatus);
        }

        void listen(uint8_t channel, int backlog, const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Local::setChannel(channel);
            Super::getConfig().setBacklog(backlog);

            listen(onStatus);
        }

        void listen(const std::string& btAddress,
                    uint8_t channel,
                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Local::setBtAddress(btAddress).setChannel(channel);

            listen(onStatus);
        }

        void listen(const std::string& btAddress,
                    uint8_t channel,
                    int backlog,
                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::getConfig().Local::setBtAddress(btAddress).setChannel(channel);
            Super::getConfig().setBacklog(backlog);

            listen(onStatus);
        }
    };

} // namespace net::rc::stream

#endif // NET_RC_STREAM_SOCKETSERVER_H
