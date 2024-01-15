/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>
#include <sys/socket.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::config {

    ConfigPhysicalSocket::ConfigPhysicalSocket(ConfigInstance* instance)
        : ConfigSection(instance, "socket", "Configuration of socket behavior") {
        add_socket_option(reuseAddressOpt, //
                          "--reuse-address{true}",
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          "Reuse socket address",
                          "bool",
                          XSTR(REUSE_ADDRESS),
                          CLI::IsMember({"true", "false"}));

        add_flag(retryOpt, //
                 "--retry{true}",
                 "Automatically retry listen|connect",
                 "bool",
                 XSTR(RETRY),
                 CLI::IsMember({"true", "false"}));

        add_flag_function(
            retryOnFatalOpt, //
            "--retry-on-fatal{true}",
            [this](int64_t) -> void {
                if (retryOnFatalOpt->as<bool>() && !retryOpt->as<bool>()) {
                    throw CLI::RequiresError(retryOnFatalOpt->get_name(), retryOpt->get_name().append("=true"));
                }
            },
            "Retry also on fatal errors",
            "bool",
            XSTR(RETRY_ON_FATAL),
            CLI::IsMember({"true", "false"}));
        retryOnFatalOpt->needs(retryOpt);

        add_option(retryTimeoutOpt, //
                   "--retry-timeout",
                   "Timeout of the retry timer",
                   "sec",
                   RETRY_TIMEOUT,
                   CLI::NonNegativeNumber);
        retryTimeoutOpt->needs(retryOpt);

        add_option(retryTriesOpt, //
                   "--retry-tries",
                   "Number of retry attempts before giving up (0 = unlimited)",
                   "tries",
                   RETRY_TRIES,
                   CLI::TypeValidator<unsigned int>());
        retryTriesOpt->needs(retryOpt);

        add_option(retryBaseOpt, //
                   "--retry-base",
                   "Base of exponential backoff",
                   "base",
                   RETRY_BASE,
                   CLI::PositiveNumber);
        retryBaseOpt->needs(retryOpt);

        add_option(retryJitterOpt, //
                   "--retry-jitter",
                   "Maximum jitter in percent to apply randomly to calculated retry timeout (0 to disable)",
                   "jitter",
                   RETRY_JITTER,
                   CLI::Range(0., 100.));
        retryJitterOpt->needs(retryOpt);

        add_option(retryLimitOpt, //
                   "--retry-limit",
                   "Upper limit in seconds of retry timeout",
                   "sec",
                   RETRY_LIMIT,
                   CLI::NonNegativeNumber);
        retryLimitOpt->needs(retryOpt);
    }

    const std::map<int, const net::phy::PhysicalSocketOption>& ConfigPhysicalSocket::getSocketOptions() {
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
                    socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, 1)});
                } else {
                    socketOptionsMap.erase(optName);
                }
            },
            description,
            typeName,
            defaultValue,
            validator);
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, int optValue) {
        socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue)});

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::string& optValue) {
        socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue)});

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::vector<char>& optValue) {
        socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue)});

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::removeSocketOption(int optName) {
        socketOptionsMap.erase(optName);

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setReuseAddress(bool reuseAddress) {
        if (reuseAddress) {
            addSocketOption(SOL_SOCKET, SO_REUSEADDR, 1);
        } else {
            removeSocketOption(SO_REUSEADDR);
        }

        reuseAddressOpt //
            ->default_val(reuseAddress ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigPhysicalSocket::getReuseAddress() const {
        return reuseAddressOpt->as<bool>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetry(bool retry) {
        retryOpt //
            ->default_val(retry ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigPhysicalSocket::getRetry() const {
        return retryOpt->as<bool>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetryOnFatal(bool retry) {
        retryOnFatalOpt //
            ->default_val(retry ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigPhysicalSocket::getRetryOnFatal() const {
        return retryOnFatalOpt->as<bool>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetryTimeout(double sec) {
        retryTimeoutOpt //
            ->default_val(sec)
            ->clear();

        return *this;
    }

    double ConfigPhysicalSocket::getRetryTimeout() const {
        return retryTimeoutOpt->as<double>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetryTries(unsigned int tries) {
        retryTriesOpt //
            ->default_val(tries)
            ->clear();

        return *this;
    }

    unsigned int ConfigPhysicalSocket::getRetryTries() const {
        return retryTriesOpt->as<unsigned int>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetryBase(double base) {
        retryBaseOpt //
            ->default_val(base)
            ->clear();

        return *this;
    }

    double ConfigPhysicalSocket::getRetryBase() const {
        return retryBaseOpt->as<double>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetryLimit(unsigned int limit) {
        retryLimitOpt //
            ->default_val(limit)
            ->clear();

        return *this;
    }

    unsigned int ConfigPhysicalSocket::getRetryLimit() const {
        return retryLimitOpt->as<unsigned int>();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::setRetryJitter(double percent) {
        retryJitterOpt //
            ->default_val(percent)
            ->clear();

        return *this;
    }

    double ConfigPhysicalSocket::getRetryJitter() const {
        return retryJitterOpt->as<double>();
    }

} // namespace net::config
