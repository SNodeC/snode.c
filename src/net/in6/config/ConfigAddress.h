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

#ifndef NET_IN6_CONFIG_CONFIGADDRESS_H
#define NET_IN6_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"  // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.h" // IWYU pragma: keep
#include "net/in6/SocketAddress.h"

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

namespace net::in6::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::in6::SocketAddress> {
        using SocketAddress = net::in6::SocketAddress;
        using Super = ConfigAddressTypeT<SocketAddress>;

    public:
        explicit ConfigAddress(net::config::ConfigInstance* instance);

    private:
        SocketAddress* init() final;

    public:
        SocketAddress newSocketAddress(SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen);

        ConfigAddress& setSocketAddress(const SocketAddress& socketAddress);

        ConfigAddress& setHost(const std::string& ipOrHostname);
        std::string getHost() const;

        ConfigAddress& setPort(uint16_t port);
        uint16_t getPort() const;

    protected:
        ConfigAddress& setIpv4Mapped(bool ipv4Mapped = true);
        bool getIpv4Mapped() const;

        ConfigAddress& setAiFlags(int aiFlags);
        int getAiFlags() const;

        ConfigAddress& setAiSockType(int aiSocktype);
        int getAiSockType() const;

        ConfigAddress& setAiProtocol(int aiProtocol);
        int getAiProtocol() const;

        ConfigAddress& setHostRequired(bool required = true);
        ConfigAddress& setPortRequired(bool required = true);

    private:
        CLI::Option* hostOpt = nullptr;
        CLI::Option* portOpt = nullptr;
        CLI::Option* numericOpt = nullptr;
        CLI::Option* ipv4MappedOpt = nullptr;

        int aiFlags = 0;
        int aiSockType = 0;
        int aiProtocol = 0;
    };

} // namespace net::in6::config

extern template class net::config::ConfigAddress<net::in6::SocketAddress>;
extern template class net::config::ConfigAddressLocal<net::in6::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::in6::SocketAddress>;
extern template class net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>;

#endif // NET_IN6_CONFIG_CONFIGADDRESS_H
