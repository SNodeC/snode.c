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

#include "ConfigPhysicalSocket.h"

#include "net/config/ConfigSection.hpp"
#include "utils/ResetToDefault.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <stdexcept>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigPhysicalSocket::ConfigPhysicalSocket(ConfigInstance* instance)
        : ConfigSection(instance, "socket", "Options for socket behaviour") {
    }

    const std::map<int, core::socket::PhysicalSocketOption>& ConfigPhysicalSocket::getSocketOptions() {
        return socketOptionsMap;
    }

    CLI::Option* ConfigPhysicalSocket::add_socket_option(CLI::Option*& opt,
                                                         const std::string& name,
                                                         int optLevel,
                                                         int optName,
                                                         const std::string& description,
                                                         const std::string& typeName,
                                                         const std::string& defaultValue,
                                                         const CLI::Validator& validator) {
        return net::config::ConfigPhysicalSocket::add_flag_function(
            opt,
            name,
            [this, &opt, optLevel, optName](int64_t) -> void {
                if (opt->as<bool>()) {
                    socketOptionsMap[optName] = core::socket::PhysicalSocketOption(optLevel, optName, 1);
                }

                (utils::ResetToDefault(opt))(opt->as<std::string>());
            },
            description,
            typeName,
            defaultValue,
            validator);
    }

    void ConfigPhysicalSocket::addSocketOption(int optName, int optLevel) {
        socketOptionsMap[optName] = core::socket::PhysicalSocketOption(optLevel, optName, 1);
    }

    void ConfigPhysicalSocket::removeSocketOption(int optName) {
        socketOptionsMap.erase(optName);
    }

} // namespace net::config
