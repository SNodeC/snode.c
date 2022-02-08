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

#ifndef NET_CONFIGREMOTE_HPP
#define NET_CONFIGREMOTE_HPP

#include "ConfigRemote.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename SocketAddress>
    ConfigRemote<SocketAddress>::ConfigRemote(CLI::App* baseSc) {
        addressSc = baseSc->add_subcommand("remote");
        addressSc->description("Connect options");
        addressSc->configurable();
    }

    template <typename SocketAddress>
    const SocketAddress& ConfigRemote<SocketAddress>::getRemoteAddress() {
        if (!initialized) {
            utils::Config::instance().parse(true); // Try command line parsing in case Address is not initialized using setLocalAddress

            address = getAddress();
            initialized = true;
        } else if (!updated) {
            updateFromCommandLine();
            updated = true;
        }

        return address;
    }

    template <typename SocketAddress>
    void ConfigRemote<SocketAddress>::setRemoteAddress(const SocketAddress& remoteAddress) {
        this->address = remoteAddress;
        this->initialized = true;
    }

    template <typename SocketAddressT>
    void ConfigRemote<SocketAddressT>::require(CLI::Option* opt) {
        addressSc->required();
        opt->required();
    }

    template <typename SocketAddressT>
    void ConfigRemote<SocketAddressT>::require(CLI::Option* opt1, CLI::Option* opt2) {
        addressSc->required();
        opt1->required();
        opt2->required();
    }

} // namespace net::config

#endif // NET_CONFIGREMOTE_HPP
