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

#ifndef NET_UN_CONFIG_CONFIGADDRESS_H
#define NET_UN_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"  // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.h" // IWYU pragma: keep
#include "net/un/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
}

// IWYU pragma: no_include "net/config/ConfigAddressLocal.hpp"
// IWYU pragma: no_include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::un::SocketAddress> {
        using SocketAddress = net::un::SocketAddress;
        using Super = ConfigAddressTypeT<SocketAddress>;

    public:
        explicit ConfigAddress(net::config::ConfigInstance* instance);

    private:
        SocketAddress* init() final;

    public:
        void setSocketAddress(const SocketAddress& socketAddress) final;

        std::string getSunPath();
        ConfigAddress& setSunPath(const std::string& sunPath);

    protected:
        void sunPathRequired();

    private:
        CLI::Option* sunPathOpt = nullptr;
    };

} // namespace net::un::config

extern template class net::un::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::un::config::ConfigAddress<net::config::ConfigAddressRemote>;

extern template class net::config::ConfigAddressLocal<net::un::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::un::SocketAddress>;

#endif // NET_L2_CONFIG_CONFIGADDRESS_H
