/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef NET_IN6_CONFIG_CONFIGADDRESS_H
#define NET_IN6_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"
#include "net/config/ConfigAddressRemote.h"
#include "net/config/ConfigAddressReverse.h"
#include "net/config/ConfigSection.h"
#include "net/in6/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
}

// IWYU pragma: no_include "net/config/ConfigAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddressReverse
        : protected net::config::ConfigSection
        , public ConfigAddressTypeT<net::in6::SocketAddress> {
    private:
        using Super = ConfigAddressTypeT<SocketAddress>;

    protected:
        explicit ConfigAddressReverse(net::config::ConfigInstance* instance,
                                      const std::string& addressOptionName,
                                      const std::string& addressOptionDescription);

    public:
        SocketAddress getSocketAddress(const SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen);

        ConfigAddressReverse& setNumericReverse(bool numeric = true);
        bool getNumericReverse() const;

    private:
        CLI::Option* numericReverseOpt = nullptr;
    };

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress
        : protected net::config::ConfigSection
        , public ConfigAddressTypeT<net::in6::SocketAddress> {
    private:
        using Super = ConfigAddressTypeT<SocketAddress>;

    protected:
        ConfigAddress(net::config::ConfigInstance* instance,
                      const std::string& addressOptionName,
                      const std::string& addressOptionDescription);

    private:
        SocketAddress* init() final;

    public:
        using Super::getSocketAddress;
        SocketAddress getSocketAddress(const SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen);

        ConfigAddress& setSocketAddress(const SocketAddress& socketAddress);

        ConfigAddress& setHost(const std::string& ipOrHostname);
        std::string getHost() const;

        ConfigAddress& setPort(uint16_t port);
        uint16_t getPort() const;

        ConfigAddress& setNumeric(bool numeric = true);
        bool getNumeric() const;

        ConfigAddress& setNumericReverse(bool numeric = true);
        bool getNumericReverse() const;

        void configurable(bool configurable = true) final;

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
        CLI::Option* numericReverseOpt = nullptr;
        CLI::Option* ipv4MappedOpt = nullptr;

        int aiFlags = 0;
        int aiSockType = 0;
        int aiProtocol = 0;
    };

} // namespace net::in6::config

extern template class net::config::ConfigAddress<net::in6::SocketAddress>;
extern template class net::config::ConfigAddressLocal<net::in6::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::in6::SocketAddress>;
extern template class net::config::ConfigAddressReverse<net::in6::SocketAddress>;
extern template class net::config::ConfigAddressBase<net::in6::SocketAddress>;
extern template class net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>;
extern template class net::in6::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;

#endif // NET_IN6_CONFIG_CONFIGADDRESS_H
