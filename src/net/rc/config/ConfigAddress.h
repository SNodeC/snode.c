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

#ifndef NET_RC_CONFIG_CONFIGADDRESS_H
#define NET_RC_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"  // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.h" // IWYU pragma: keep
#include "net/rc/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
}

// IWYU pragma: no_include "net/config/ConfigAddressLocal.hpp"
// IWYU pragma: no_include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::rc::SocketAddress> {
        using SocketAddress = net::rc::SocketAddress;
        using Super = ConfigAddressTypeT<SocketAddress>;

    public:
        explicit ConfigAddress(net::config::ConfigInstance* instance);

        SocketAddress getAddress() const final;
        void setAddress(const SocketAddress& socketAddress) final;

        std::string getBtAddress();
        void setBtAddress(const std::string& btAddress);

        uint8_t getChannel();
        void setChannel(uint8_t channel);

    protected:
        void hostRequired();
        void channelRequired();

    private:
        using Super::required;

        CLI::Option* hostOpt = nullptr;
        CLI::Option* channelOpt = nullptr;
    };

} // namespace net::rc::config

extern template class net::rc::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::rc::config::ConfigAddress<net::config::ConfigAddressRemote>;

extern template class net::config::ConfigAddressLocal<net::rc::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::rc::SocketAddress>;

#endif // NET_RC_CONFIG_CONFIGADDRESS_H
