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

#ifndef NET_IN_STREAM_CONFIG_CONFIGSOCKETSERVER_H
#define NET_IN_STREAM_CONFIG_CONFIGSOCKETSERVER_H

#include "net/config/stream/ConfigSocketServer.h" // IWYU pragma: export
#include "net/in/config/ConfigAddress.h"          // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream::config {

    class ConfigSocketServer : public net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress> {
        using Super = net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress>;

    public:
        using Remote = ConfigSocketServer;

        explicit ConfigSocketServer(net::config::ConfigInstance* instance);

        ~ConfigSocketServer() override;

        SocketAddress newSocketAddress(const SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen);

        void setReusePort(bool reusePort = true);
        bool getReusePort() const;

        ConfigSocketServer& setNumericReverse(bool numeric = true);
        bool getNumericReverse() const;

    private:
        CLI::Option* reusePortOpt = nullptr;
        CLI::Option* numericReverseOpt = nullptr;
    };
} // namespace net::in::stream::config

extern template class net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress>;

#endif // NET_IN_STREAM_CONFIG_CONFIGSOCKETSERVER_H
