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

#include "net/in6/config/ConfigAddress.h"

#include "net/config/ConfigAddressBase.hpp"    // IWYU pragma: keep
#include "net/config/ConfigAddressLocal.hpp"   // IWYU pragma: keep
#include "net/config/ConfigAddressRemote.hpp"  // IWYU pragma: keep
#include "net/config/ConfigAddressReverse.hpp" // IWYU pragma: keep
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/netdb.h"
#include "utils/PreserveErrno.h"

#include <limits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::in6::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddressReverse<ConfigAddressType>::ConfigAddressReverse(net::config::ConfigInstance* instance,
                                                                  [[maybe_unused]] const std::string& addressOptionName,
                                                                  [[maybe_unused]] const std::string& addressOptionDescription)
        : ConfigSection(instance, this)
        , ConfigAddressType<net::in6::SocketAddress>(this) {
        numericReverseOpt = addFlag( //
            "--numeric-reverse",
            "Suppress reverse host name lookup",
            "bool",
            XSTR(IN6_NUMERIC_REVERSE),
            CLI::IsMember({"true", "false"}));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddressReverse<ConfigAddressType>::getSocketAddress(const SocketAddress::SockAddr& sockAddr,
                                                                            SocketAddress::SockLen sockAddrLen) {
        SocketAddress socketAddress;
        try {
            socketAddress = SocketAddress(sockAddr, sockAddrLen, numericReverseOpt->as<bool>());
        } catch ([[maybe_unused]] const SocketAddress::BadSocketAddress& badSocketAddress) {
            try {
                socketAddress = Super::getSocketAddress(sockAddr, sockAddrLen);
            } catch ([[maybe_unused]] const SocketAddress::BadSocketAddress& badSocketAddress) {
                throw;
            }
        }

        return socketAddress;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddressReverse<ConfigAddressType>& ConfigAddressReverse<ConfigAddressType>::setNumericReverse(bool numeric) {
        numericReverseOpt //
            ->default_str(numeric ? "true" : "false")
            ->clear();

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    bool ConfigAddressReverse<ConfigAddressType>::getNumericReverse() const {
        return numericReverseOpt->as<bool>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance,
                                                    [[maybe_unused]] const std::string& addressOptionName,
                                                    [[maybe_unused]] const std::string& addressOptionDescription)
        : ConfigSection(instance, this)
        , ConfigAddressType<net::in6::SocketAddress>(this) {
        hostOpt = addOption( //
            "--host",
            "Host name or IPv6 address",
            "hostname|IPv6",
            "::",
            CLI::TypeValidator<std::string>());
        portOpt = addOption( //
            "--port",
            "Port number",
            "port",
            0,
            CLI::Range(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()));
        numericOpt = addFlag( //
            "--numeric",
            "Suppress host name lookup",
            "bool",
            XSTR(IN6_NUMERIC),
            CLI::IsMember({"true", "false"}));
        numericReverseOpt = addFlag( //
            "--numeric-reverse",
            "Suppress reverse host name lookup",
            "bool",
            XSTR(IN6_NUMERIC_REVERSE),
            CLI::IsMember({"true", "false"}));
        ipv4MappedOpt = addFlag( //
            "--ipv4-mapped",
            "Resolve IPv4-mapped IPv6 addresses also",
            "bool",
            XSTR(IN6_IPV4_MAPPED),
            CLI::IsMember({"true", "false"}));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        SocketAddress* socketAddress = new SocketAddress(hostOpt->as<std::string>(), portOpt->as<uint16_t>());

        try {
            socketAddress->init({.aiFlags = (aiFlags & ~AI_NUMERICHOST) | (numericOpt->as<bool>() ? AI_NUMERICHOST : 0),
                                 .aiSockType = aiSockType,
                                 .aiProtocol = aiProtocol});
        } catch (const core::socket::SocketAddress::BadSocketAddress&) {
            delete socketAddress;
            socketAddress = nullptr;

            throw;
        }

        return socketAddress;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getSocketAddress(const SocketAddress::SockAddr& sockAddr,
                                                                     SocketAddress::SockLen sockAddrLen) {
        SocketAddress socketAddress;
        try {
            socketAddress = SocketAddress(sockAddr, sockAddrLen, numericReverseOpt->as<bool>());
        } catch ([[maybe_unused]] const SocketAddress::BadSocketAddress& badSocketAddress) {
            try {
                socketAddress = Super::getSocketAddress(sockAddr, sockAddrLen);
            } catch ([[maybe_unused]] const SocketAddress::BadSocketAddress& badSocketAddress) {
                throw;
            }
        }

        return socketAddress;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        setHost(socketAddress.getHost());
        setPort(socketAddress.getPort());

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setHost(const std::string& ipOrHostname) {
        const utils::PreserveErrno preserveErrno;

        hostOpt //
            ->default_val(ipOrHostname)
            ->clear();
        this->required(hostOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getHost() const {
        return hostOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPort(uint16_t port) {
        const utils::PreserveErrno preserveErrno;

        portOpt //
            ->default_val(port)
            ->clear();
        this->required(portOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint16_t ConfigAddress<ConfigAddressType>::getPort() const {
        return portOpt->as<uint16_t>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setNumeric(bool numeric) {
        const utils::PreserveErrno preserveErrno;

        numericOpt //
            ->default_str(numeric ? "true" : "false")
            ->clear();

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    bool ConfigAddress<ConfigAddressType>::getNumeric() const {
        return numericOpt->as<bool>();
    }

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    void ConfigAddress<ConfigAddressTypeT>::configurable(bool configurable) {
        this->setConfigurable(hostOpt, configurable);
        this->setConfigurable(portOpt, configurable);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setNumericReverse(bool numeric) {
        numericReverseOpt //
            ->default_str(numeric ? "true" : "false")
            ->clear();

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setIpv4Mapped(bool ipv4Mapped) {
        const utils::PreserveErrno preserveErrno;

        ipv4MappedOpt //
            ->default_val(ipv4Mapped)
            ->clear();

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    bool ConfigAddress<ConfigAddressType>::getIpv4Mapped() const {
        return ipv4MappedOpt->as<bool>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setAiFlags(int aiFlags) {
        this->aiFlags = aiFlags;

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    int ConfigAddress<ConfigAddressType>::getAiFlags() const {
        return aiFlags;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setAiSockType(int aiSockType) {
        this->aiSockType = aiSockType;

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    int ConfigAddress<ConfigAddressType>::getAiSockType() const {
        return aiSockType;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setAiProtocol(int aiProtocol) {
        this->aiProtocol = aiProtocol;

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    int ConfigAddress<ConfigAddressType>::getAiProtocol() const {
        return aiProtocol;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setHostRequired(bool required) {
        this->required(hostOpt, required);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPortRequired(bool required) {
        this->required(portOpt, required);

        return *this;
    }

} // namespace net::in6::config

template class net::config::ConfigAddress<net::in6::SocketAddress>;
template class net::config::ConfigAddressLocal<net::in6::SocketAddress>;
template class net::config::ConfigAddressRemote<net::in6::SocketAddress>;
template class net::config::ConfigAddressReverse<net::in6::SocketAddress>;
template class net::config::ConfigAddressBase<net::in6::SocketAddress>;
template class net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>;
template class net::in6::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;
