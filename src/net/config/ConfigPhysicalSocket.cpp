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

#include "net/config/ConfigInstance.h"
#include "net/config/ConfigSection.hpp"
#include "utils/ResetToDefault.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <stdexcept>
#include <string>
#include <sys/socket.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigPhysicalSocket::ConfigPhysicalSocket(ConfigInstance* instance)
        : ConfigSection(instance, "socket", "Configuration of socket behaviour for instance '" + instance->getInstanceName() + "'") {
        add_socket_option(reuseAddressOpt, //
                          "--reuse-address",
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          "Reuse socket address",
                          "bool",
                          "false",
                          CLI::IsMember({"true", "false"}));

        add_flag_function(
            retryOpt, //
            "--retry",
            [this](int64_t) -> void {
                if (!this->retryOpt->as<bool>()) {
                    this->retryTimeoutOpt->clear();
                    this->retryTriesOpt->clear();
                }
                utils::ResetToDefault(this->retryOpt)(this->retryOpt->as<bool>());
            },
            "Automatically retry listen|connect",
            "bool",
            "false",
            CLI::IsMember({"true", "false"}));

        add_option(retryTimeoutOpt, //
                   "--retry-timeout",
                   "Timeout of the retry timer",
                   "sec",
                   1,
                   CLI::TypeValidator<unsigned int>());
        retryTimeoutOpt->needs(retryOpt);

        add_option(retryTriesOpt, //
                   "--retry-tries",
                   "Number of retry attempts bevore giving up (0 = unlimited)",
                   "tries",
                   0,
                   CLI::TypeValidator<unsigned int>());
        retryTriesOpt->needs(retryOpt);
    }

    const std::map<int, const PhysicalSocketOption>& ConfigPhysicalSocket::getSocketOptions() {
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
        return net::config::ConfigSection::add_flag_function(
            opt,
            name,
            [this, &opt, optLevel, optName](int64_t) -> void {
                if (opt->as<bool>()) {
                    socketOptionsMap.insert({optName, net::PhysicalSocketOption(optLevel, optName, 1)});
                } else {
                    socketOptionsMap.erase(optName);
                }

                (utils::ResetToDefault(opt))(opt->as<std::string>());
            },
            description,
            typeName,
            defaultValue,
            validator);
    }

    void ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, int optValue) {
        socketOptionsMap.insert({optName, net::PhysicalSocketOption(optLevel, optName, optValue)});
    }

    void ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::string& optValue) {
        socketOptionsMap.insert({optName, net::PhysicalSocketOption(optLevel, optName, optValue)});
    }
    void ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::vector<char>& optValue) {
        socketOptionsMap.insert({optName, net::PhysicalSocketOption(optLevel, optName, optValue)});
    }

    void ConfigPhysicalSocket::removeSocketOption(int optName) {
        socketOptionsMap.erase(optName);
    }

    void ConfigPhysicalSocket::setReuseAddress(bool reuseAddress) {
        if (reuseAddress) {
            addSocketOption(SOL_SOCKET, SO_REUSEADDR, 1);
        } else {
            removeSocketOption(SO_REUSEADDR);
        }

        reuseAddressOpt //
            ->default_val(reuseAddress ? "true" : "false")
            ->take_all()
            ->clear();
    }

    bool ConfigPhysicalSocket::getReuseAddress() {
        return reuseAddressOpt->as<bool>();
    }

    void ConfigPhysicalSocket::setRetry(bool retry) {
        retryOpt //
            ->default_val(retry)
            ->clear();
    }

    bool ConfigPhysicalSocket::getRetry() {
        return retryOpt->as<bool>();
    }

    void ConfigPhysicalSocket::setRetryTimeout(unsigned int sec) {
        retryTimeoutOpt //
            ->default_val(sec)
            ->clear();
    }

    unsigned int ConfigPhysicalSocket::getRetryTimeout() {
        return retryTimeoutOpt->as<unsigned int>();
    }

    void ConfigPhysicalSocket::setRetryTries(unsigned int tries) {
        retryTriesOpt //
            ->default_val(tries)
            ->clear();
    }

    unsigned int ConfigPhysicalSocket::getRetryTries() {
        return retryTriesOpt->as<unsigned int>();
    }

} // namespace net::config
