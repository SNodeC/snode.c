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

#ifndef NET_IN_CONFIG_CONFIGADDRESS_H
#define NET_IN_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"  // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.h" // IWYU pragma: keep
#include "net/in/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
} // namespace net::config

// IWYU pragma: no_include "net/config/ConfigAddressLocal.hpp"
// IWYU pragma: no_include "net/config/ConfigAddressRemote.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddressReverse : public ConfigAddressTypeT<net::in::SocketAddress> {
    public:
        using Super = ConfigAddressTypeT<SocketAddress>;

        explicit ConfigAddressReverse(net::config::ConfigInstance* instance,
                                      const std::string& addressOptionName,
                                      const std::string& addressOptionDescription);

        SocketAddress newSocketAddress(const SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen);

        ConfigAddressReverse& setNumericReverse(bool numeric = true);
        bool getNumericReverse() const;

    private:
        CLI::Option* numericReverseOpt = nullptr;
    };

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::in::SocketAddress> {
    public:
        using Super = ConfigAddressTypeT<net::in::SocketAddress>;

        explicit ConfigAddress(net::config::ConfigInstance* instance,
                               const std::string& addressOptionName,
                               const std::string& addressOptionDescription);

    private:
        SocketAddress* init() final;

    public:
        SocketAddress newSocketAddress(const SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen);

        ConfigAddress& setSocketAddress(const SocketAddress& socketAddress);

        ConfigAddress& setHost(const std::string& ipOrHostname);
        std::string getHost() const;

        ConfigAddress& setPort(uint16_t port);
        uint16_t getPort() const;

        ConfigAddress& setNumeric(bool numeric = true);
        bool getNumeric() const;

        ConfigAddress& setNumericReverse(bool numeric = true);
        bool getNumericReverse() const;

    protected:
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
        CLI::Option* numericReverseOpt = nullptr;

        int aiFlags = 0;
        int aiSockType = 0;
        int aiProtocol = 0;
    };

} // namespace net::in::config

extern template class net::config::ConfigAddress<net::in::SocketAddress>;
extern template class net::config::ConfigAddressLocal<net::in::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::in::SocketAddress>;
extern template class net::config::ConfigAddressReverse<net::in::SocketAddress>;
extern template class net::in::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::in::config::ConfigAddress<net::config::ConfigAddressRemote>;
extern template class net::in::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;

#endif // NET_IN_CONFIG_CONFIGADDRESS_H
