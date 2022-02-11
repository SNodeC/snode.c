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

#ifndef NET_IN6_CONFIG_CONFIGADDRESS_H
#define NET_IN6_CONFIG_CONFIGADDRESS_H

#include "net/in6/SocketAddress.h"

// IWYU pragma: no_include "net/config/ConfigAddressLocal.hpp"
// IWYU pragma: no_include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::in6::SocketAddress> {
        using SocketAddress = net::in6::SocketAddress;
        using ConfigAddressType = ConfigAddressTypeT<SocketAddress>;

    public:
        explicit ConfigAddress(CLI::App* baseSc);

    protected:
        void required();
        void portRequired();

        CLI::Option* hostOpt = nullptr;
        CLI::Option* portOpt = nullptr;

    private:
        void updateFromCommandLine() override;

        std::string host{};
        uint16_t port{};
    };

} // namespace net::in6::config

#endif // NET_IN6_CONFIG_CONFIGADDRESS_H
