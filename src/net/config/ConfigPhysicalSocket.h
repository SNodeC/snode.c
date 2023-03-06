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

#ifndef NET_CONFIG_CONFIGPHYSICALSOCKET_H
#define NET_CONFIG_CONFIGPHYSICALSOCKET_H

#include "core/socket/PhysicalSocketOption.h"
#include "net/config/ConfigSection.h" // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

namespace CLI {
    class Option;
    class Validator;
} // namespace CLI

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    class ConfigPhysicalSocket : protected ConfigSection {
    public:
        explicit ConfigPhysicalSocket(ConfigInstance* instance);

        const std::map<int, const core::socket::PhysicalSocketOption>& getSocketOptions();

    protected:
        CLI::Option* add_socket_option(CLI::Option*& opt,
                                       const std::string& name,
                                       int optLevel,
                                       int optName,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const std::string& defaultValue,
                                       const CLI::Validator& validator);

        void addSocketOption(int optName, int optLevel);
        void removeSocketOption(int optName);

        std::map<int, const core::socket::PhysicalSocketOption> socketOptionsMap; // key is optName, value is optLevel
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGPHYSICALSOCKET_H
