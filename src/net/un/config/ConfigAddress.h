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

#ifndef NET_UN_CONFIG_CONFIGADDRESS_H
#define NET_UN_CONFIG_CONFIGADDRESS_H

#include "net/un/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::un::SocketAddress> {
        using SocketAddress = net::un::SocketAddress;
        using ConfigAddressType = ConfigAddressTypeT<SocketAddress>;

    public:
        explicit ConfigAddress(CLI::App* baseSc);

    protected:
        void required();
        void sunPathRequired();

        CLI::Option* sunPathOpt = nullptr;

    private:
        SocketAddress getAddress() const override;

        void updateFromCommandLine() override;

        std::string sunPath{};
    };

} // namespace net::un::config

#endif // NET_L2_CONFIG_CONFIGADDRESS_H
