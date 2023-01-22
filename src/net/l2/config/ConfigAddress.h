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

#ifndef NET_L2_CONFIG_CONFIGADDRESS_H
#define NET_L2_CONFIG_CONFIGADDRESS_H

#include "net/l2/SocketAddress.h"

// IWYU pragma: no_include "net/config/ConfigAddressLocal.hpp"
// IWYU pragma: no_include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::config {

    using SocketAddress = net::l2::SocketAddress;

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<SocketAddress> {
        using Super = ConfigAddressTypeT<SocketAddress>;

    public:
        ConfigAddress();

    protected:
        void required();
        void psmRequired();

    private:
        SocketAddress getAddress() final;
        void setAddress(const SocketAddress& socketAddress) final;

        CLI::Option* hostOpt = nullptr;
        CLI::Option* psmOpt = nullptr;
    };

} // namespace net::l2::config

#endif // NET_L2_CONFIG_CONFIGADDRESS_H
