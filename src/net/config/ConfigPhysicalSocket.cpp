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
#include "utils/ResetToDefault.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <stdexcept>
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

        add_flag_function(
            retryOpt, //
            "--retry{true}",
            [this](int64_t) -> void {
                if (!this->retryOpt->as<bool>()) {
                    this->retryOnFatalOpt->clear();
                    this->retryTimeoutOpt->clear();
                    this->retryTriesOpt->clear();
                    this->retryBaseOpt->clear();
                    this->retryJitterOpt->clear();
                    this->retryLimitOpt->clear();
                }
                (utils::ResetToDefault(retryOpt))(retryOpt->as<std::string>());
            },
            "Automatically retry listen|connect",
            "bool",
            XSTR(RETRY),
            CLI::IsMember({"true", "false"}));

        add_flag(retryOnFatalOpt, //
                 "--retry-on-fatal{true}",
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

                (utils::ResetToDefault(opt))(opt->as<std::string>());
            },
            description,
            typeName,
            defaultValue,
            validator);
    }

    void ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, int optValue) {
        socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue)});
    }

    void ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::string& optValue) {
        socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue)});
    }
    void ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::vector<char>& optValue) {
        socketOptionsMap.insert({optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue)});
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

    bool ConfigPhysicalSocket::getReuseAddress() const {
        return reuseAddressOpt->as<bool>();
    }

    void ConfigPhysicalSocket::setRetry(bool retry) {
        retryOpt //
            ->default_val(retry ? "true" : "false")
            ->clear();
    }

    bool ConfigPhysicalSocket::getRetry() const {
        return retryOpt->as<bool>();
    }

    void ConfigPhysicalSocket::setRetryOnFatal(bool retry) {
        retryOnFatalOpt //
            ->default_val(retry ? "true" : "false")
            ->clear();
    }

    bool ConfigPhysicalSocket::getRetryOnFatal() const {
        return retryOnFatalOpt->as<bool>();
    }

    void ConfigPhysicalSocket::setRetryTimeout(double sec) {
        retryTimeoutOpt //
            ->default_val(sec)
            ->clear();
    }

    double ConfigPhysicalSocket::getRetryTimeout() const {
        return retryTimeoutOpt->as<double>();
    }

    void ConfigPhysicalSocket::setRetryTries(unsigned int tries) {
        retryTriesOpt //
            ->default_val(tries)
            ->clear();
    }

    unsigned int ConfigPhysicalSocket::getRetryTries() const {
        return retryTriesOpt->as<unsigned int>();
    }

    void ConfigPhysicalSocket::setRetryBase(double base) {
        retryBaseOpt //
            ->default_val(base)
            ->clear();
    }

    double ConfigPhysicalSocket::getRetryBase() const {
        return retryBaseOpt->as<double>();
    }

    void ConfigPhysicalSocket::setRetryLimit(unsigned int limit) {
        retryLimitOpt //
            ->default_val(limit)
            ->clear();
    }

    unsigned int ConfigPhysicalSocket::getRetryLimit() const {
        return retryLimitOpt->as<unsigned int>();
    }

    void ConfigPhysicalSocket::setRetryJitter(double percent) {
        retryJitterOpt //
            ->default_val(percent)
            ->clear();
    }

    double ConfigPhysicalSocket::getRetryJitter() const {
        return retryJitterOpt->as<double>();
    }

} // namespace net::config
