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

#ifndef NET_CONFIG_STREAM_CONFIGSOCKETCLIENT_H
#define NET_CONFIG_STREAM_CONFIGSOCKETCLIENT_H

#include "net/config/ConfigAddressLocal.h"         // IWYU pragma: export
#include "net/config/ConfigAddressRemote.h"        // IWYU pragma: export
#include "net/config/ConfigConnection.h"           // IWYU pragma: export
#include "net/config/ConfigPhysicalSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config::stream {

    template <template <template <typename SocketAddress> typename ConfigAddressTypeT> typename ConfigAddressLocalT,
              template <template <typename SocketAddress> typename ConfigAddressTypeT> typename ConfigAddressRemoteT = ConfigAddressLocalT>
    class ConfigSocketClient
        : public ConfigAddressRemoteT<net::config::ConfigAddressRemote>
        , public ConfigAddressLocalT<net::config::ConfigAddressLocal>
        , public net::config::ConfigConnection
        , public net::config::ConfigPhysicalSocketClient {
    public:
        using Remote = ConfigAddressRemoteT<net::config::ConfigAddressRemote>;
        using Local = ConfigAddressLocalT<net::config::ConfigAddressLocal>;

    protected:
        explicit ConfigSocketClient(net::config::ConfigInstance* instance);
    };

} // namespace net::config::stream

#endif // NET_CONFIG_STREAM_CONFIGSOCKETCLIENT_H
