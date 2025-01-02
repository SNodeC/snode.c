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

#ifndef NET_L2_CONFIG_CONFIGADDRESS_H
#define NET_L2_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"
#include "net/config/ConfigAddressRemote.h"
#include "net/config/ConfigAddressReverse.h"
#include "net/l2/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddressReverse : public ConfigAddressTypeT<net::l2::SocketAddress> {
    private:
        using Super = ConfigAddressTypeT<SocketAddress>;

    protected:
        using Super::Super;
    };

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::l2::SocketAddress> {
    private:
        using Super = ConfigAddressTypeT<net::l2::SocketAddress>;

    protected:
        explicit ConfigAddress(net::config::ConfigInstance* instance,
                               const std::string& addressOptionName,
                               const std::string& addressOptionDescription);

    private:
        SocketAddress* init() final;

    public:
        ConfigAddress& setSocketAddress(const SocketAddress& socketAddress);

        ConfigAddress& setBtAddress(const std::string& btAddress);
        std::string getBtAddress() const;

        ConfigAddress& setPsm(uint16_t psm);
        uint16_t getPsm() const;

    protected:
        ConfigAddress& setBtAddressRequired(bool required = true);
        ConfigAddress& setPsmRequired(bool required = true);

        CLI::Option* btAddressOpt = nullptr;
        CLI::Option* psmOpt = nullptr;
    };

} // namespace net::l2::config

extern template class net::config::ConfigAddress<net::l2::SocketAddress>;
extern template class net::config::ConfigAddressLocal<net::l2::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::l2::SocketAddress>;
extern template class net::config::ConfigAddressReverse<net::l2::SocketAddress>;
extern template class net::config::ConfigAddressBase<net::l2::SocketAddress>;
extern template class net::l2::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::l2::config::ConfigAddress<net::config::ConfigAddressRemote>;
extern template class net::l2::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;

#endif // NET_L2_CONFIG_CONFIGADDRESS_H
