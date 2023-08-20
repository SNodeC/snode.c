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

#ifndef NET_STREAM_CONFIG_CONFIGSOCKETSERVER_H
#define NET_STREAM_CONFIG_CONFIGSOCKETSERVER_H

#include "net/config/ConfigAddressLocal.h"   // IWYU pragma: export
#include "net/config/ConfigConnection.h"     // IWYU pragma: export
#include "net/config/ConfigListen.h"         // IWYU pragma: export
#include "net/config/ConfigPhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
}

namespace utils {
    class Timeval;
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::stream::config {

    template <template <template <typename SocketAddress> typename ConfigAddressTypeT> typename ConfigAddressT>
    class ConfigSocketServer
        : public ConfigAddressT<net::config::ConfigAddressLocal>
        , public net::config::ConfigConnection
        , public net::config::ConfigPhysicalSocket
        , public net::config::ConfigListen {
    public:
        using Local = ConfigAddressT<net::config::ConfigAddressLocal>;

        explicit ConfigSocketServer(net::config::ConfigInstance* instance);

        void setAcceptTimeout(const utils::Timeval& acceptTimeout);
        utils::Timeval getAcceptTimeout() const;

    private:
        CLI::Option* acceptTimeoutOpt = nullptr;
    };

} // namespace net::stream::config

#endif // NET_STREAM_CONFIG_CONFIGSOCKETSERVER_H
