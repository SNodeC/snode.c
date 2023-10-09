/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef NET_RC_STREAM_SOCKETCLIENT_H
#define NET_RC_STREAM_SOCKETCLIENT_H

#include "core/socket/stream/SocketClient.h"        // IWYU pragma: export
#include "net/rc/phy/stream/PhysicalSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::stream {

    template <template <typename PhysicalSocketClient, typename ConfigSocketClientT> typename SocketConnectorT,
              typename ConfigSocketClientT,
              typename SocketContextFactoryT>
    class SocketClient
        : public core::socket::stream::SocketClient<SocketConnectorT<net::rc::phy::stream::PhysicalSocketClient, ConfigSocketClientT>,
                                                    SocketContextFactoryT> {
    private:
        using Super = core::socket::stream::SocketClient<SocketConnectorT<net::rc::phy::stream::PhysicalSocketClient, ConfigSocketClientT>,
                                                         SocketContextFactoryT>;

    public:
        using Super::Super;

        using Super::connect;

        void connect(const std::string& btAddress, uint8_t channel, const std::function<void(const SocketAddress&, int)>& onError) {
            Super::getConfig().Remote::setBtAddress(btAddress).setChannel(channel);

            connect(SocketAddress(btAddress, channel), onError);
        }

        void connect(const std::string& btAddress,
                     uint8_t channel,
                     const std::string& bindBtAddress,
                     const std::function<void(const SocketAddress&, int)>& onError) {
            Super::getConfig().Remote::setBtAddress(btAddress).setChannel(channel);
            Super::getConfig().Local::setBtAddress(bindBtAddress);

            connect(onError);
        }

        void connect(const std::string& btAddress,
                     uint8_t channel,
                     uint8_t bindChannel,
                     const std::function<void(const SocketAddress&, int)>& onError) {
            Super::getConfig().Remote::setBtAddress(btAddress).setChannel(channel);
            Super::getConfig().Local::setChannel(bindChannel);

            connect(onError);
        }

        void connect(const std::string& btAddress,
                     uint8_t channel,
                     const std::string& bindBtAddress,
                     uint8_t bindChannel,
                     const std::function<void(const SocketAddress&, int)>& onError) {
            Super::getConfig().Remote::setBtAddress(btAddress).setChannel(channel);
            Super::getConfig().Local::setBtAddress(bindBtAddress).setChannel(bindChannel);

            connect(onError);
        }
    };

} // namespace net::rc::stream

#endif // NET_RC_STREAM_SOCKETCLIENT_H
