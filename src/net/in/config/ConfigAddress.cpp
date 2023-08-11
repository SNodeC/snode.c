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

#include "net/in/config/ConfigAddress.h"

#include "net/config/ConfigAddressLocal.hpp"
#include "net/config/ConfigAddressRemote.hpp"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cstdint>
#include <limits>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::config {

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>::ConfigAddress(net::config::ConfigInstance* instance)
        : Super(instance) {
        Super::add_option(hostOpt, //
                          "--host",
                          "Host name or IPv4 address",
                          "hostname|IPv4",
                          "0.0.0.0",
                          CLI::TypeValidator<std::string>());
        Super::add_option(portOpt, //
                          "--port",
                          "Port number",
                          "port",
                          0,
                          CLI::Range(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()));
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::hostRequired() {
        Super::required(hostOpt, true);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::portRequired() {
        Super::required(portOpt, true);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    SocketAddress ConfigAddress<ConfigAddressType>::getSocketAddress() const {
        utils::PreserveErrno preserveErrno;

        return SocketAddress(hostOpt->as<std::string>(), portOpt->as<uint16_t>()).setAiFlags(aiFlags);
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setSocketAddress(const SocketAddress& socketAddress) {
        VLOG(0) << "########################## setSocketAddress()";
        setIpOrHostname(socketAddress.getHost());
        setPort(socketAddress.getPort());
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    std::string ConfigAddress<ConfigAddressType>::getIpOrHostname() {
        return hostOpt->as<std::string>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setIpOrHostname(const std::string& ipOrHostname) {
        VLOG(0) << "########################## setIpOrHostname()";
        utils::PreserveErrno preserveErrno;

        hostOpt //
            ->default_val(ipOrHostname);
        Super::required(hostOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    uint16_t ConfigAddress<ConfigAddressType>::getPort() {
        return portOpt->as<uint16_t>();
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    ConfigAddress<ConfigAddressType>& ConfigAddress<ConfigAddressType>::setPort(uint16_t port) {
        VLOG(0) << "########################## setPort()";
        utils::PreserveErrno preserveErrno;

        portOpt //
            ->default_val(port);
        Super::required(portOpt, false);

        return *this;
    }

    template <template <typename SocketAddress> typename ConfigAddressType>
    void ConfigAddress<ConfigAddressType>::setAiFlags(int aiFlags) {
        this->aiFlags = aiFlags;
    }

} // namespace net::in::config

template class net::in::config::ConfigAddress<net::config::ConfigAddressLocal>;
template class net::in::config::ConfigAddress<net::config::ConfigAddressRemote>;

template class net::config::ConfigAddressLocal<net::in::SocketAddress>;
template class net::config::ConfigAddressRemote<net::in::SocketAddress>;
