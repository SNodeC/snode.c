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

#ifndef NET_CONFIG_CONFIGADDRESS_H
#define NET_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigBase.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
} // namespace CLI

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename SocketAddressT>
    class ConfigAddress : virtual public ConfigBase {
        using SocketAddress = SocketAddressT;

    protected:
        ConfigAddress(const std::string& addressOptionName, const std::string& addressOptionDescription);

        const SocketAddress& getAddress();
        void setAddress(const SocketAddress& address);

        void require(CLI::Option* opt);
        void require(CLI::Option* opt1, CLI::Option* opt2);

        bool isInitialized() const;

        CLI::App* addressSc = nullptr;
        SocketAddress address;

    private:
        virtual void updateFromCommandLine() = 0;
        virtual void addressDefaultsFromCurrent() = 0;

        bool initialized = false;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGADDRESS_H
