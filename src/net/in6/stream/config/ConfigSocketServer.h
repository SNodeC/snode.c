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

#ifndef NET_IN6_STREAM_CONFIG_CONFIGSOCKETSERVER_H
#define NET_IN6_STREAM_CONFIG_CONFIGSOCKETSERVER_H

#include "net/config/stream/ConfigSocketServer.h" // IWYU pragma: export
#include "net/in6/config/ConfigAddress.h"         // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream::config {

    class ConfigSocketServer
        : public net::config::stream::ConfigSocketServer<net::in6::config::ConfigAddress, net::in6::config::ConfigAddressReverse> {
    public:
        explicit ConfigSocketServer(net::config::ConfigInstance* instance);

        ~ConfigSocketServer() override;

        void setReusePort(bool reusePort = true);
        bool getReusePort() const;

        void setIPv6Only(bool iPv6Only = true);
        bool getIPv6Only() const;

    private:
        CLI::Option* reusePortOpt = nullptr;
        CLI::Option* iPv6OnlyOpt = nullptr;
    };

} // namespace net::in6::stream::config

extern template class net::config::stream::ConfigSocketServer<net::in6::config::ConfigAddress, net::in6::config::ConfigAddressReverse>;

#endif // NET_IN6_STREAM_CONFIG_CONFIGSOCKETSERVER_H
