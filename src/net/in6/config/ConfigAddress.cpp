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

#include "net/in6/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/netdb.h"
#include "utils/PreserveErrno.h"

#include <cstdint>
#include <limits>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::in6::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance)
        : Super(instance) {
        Super::add_option(hostOpt, //
                          "--host",
                          "Host name or IPv6 address",
                          "hostname|IPv6",
                          "::",
                          CLI::TypeValidator<std::string>());
        Super::add_option(portOpt, //
                          "--port",
                          "Port number",
                          "port",
                          0,
                          CLI::Range(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()));
        Super::add_flag(ipv4MappedOpt, //
                        "--ipv4-mapped",
                        "Resolve IPv4-mapped IPv6 addresses also",
                        "bool",
                        XSTR(IPV4_MAPPED),
                        CLI::IsMember({"true", "false"}));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress* ConfigAddress<ConfigAddressType>::init() {
        return &(new SocketAddress(hostOpt->as<std::string>(), portOpt->as<uint16_t>()))
                    ->setAiFlags(aiFlags | (ipv4MappedOpt->as<bool>() ? AI_V4MAPPED : 0))
                    .setAiSockType(aiSockType)
                    .setAiProtocol(aiProtocol)
                    .init();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        setHost(socketAddress.getHost());
        setPort(socketAddress.getPort());

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setHost(const std::string& ipOrHostname) {
        utils::PreserveErrno preserveErrno;

        hostOpt //
            ->default_val(ipOrHostname)
            ->clear();
        Super::required(hostOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getHost() const {
        return hostOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPort(uint16_t port) {
        utils::PreserveErrno preserveErrno;

        portOpt //
            ->default_val(port)
            ->clear();
        Super::required(portOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint16_t ConfigAddress<ConfigAddressType>::getPort() const {
        return portOpt->as<uint16_t>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setIpv4Mapped(bool ipv4Mapped) {
        utils::PreserveErrno preserveErrno;

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
        this->aiFlags |= aiFlags;

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
        Super::required(hostOpt, required);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPortRequired(bool required) {
        Super::required(portOpt, required);

        return *this;
    }

} // namespace net::in6::config

template class net::config::ConfigAddress<net::in6::SocketAddress>;
template class net::config::ConfigAddressLocal<net::in6::SocketAddress>;
template class net::config::ConfigAddressRemote<net::in6::SocketAddress>;
template class net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::in6::config::ConfigAddress<net::config::ConfigAddressRemote>;
