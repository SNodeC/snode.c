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

#include "log/Logger.h"

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
        reuseAddressOpt = add_socket_option( //
            "--reuse-address{true}",
            SOL_SOCKET,
            SO_REUSEADDR,
            "Reuse socket address",
            "bool",
            XSTR(REUSE_ADDRESS),
            CLI::IsMember({"true", "false"}));

        retryOpt = addFlag( //
            "--retry{true}",
            "Automatically retry listen|connect",
            "bool",
            XSTR(RETRY),
            CLI::IsMember({"true", "false"}));

        retryOnFatalOpt = addFlagFunction( //
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

        retryTimeoutOpt = addOption( //
            "--retry-timeout",
            "Timeout of the retry timer",
            "sec",
            RETRY_TIMEOUT,
            CLI::NonNegativeNumber);
        retryTimeoutOpt->needs(retryOpt);

        retryTriesOpt = addOption( //
            "--retry-tries",
            "Number of retry attempts before giving up (0 = unlimited)",
            "tries",
            RETRY_TRIES,
            CLI::TypeValidator<unsigned int>());
        retryTriesOpt->needs(retryOpt);

        retryBaseOpt = addOption( //
            "--retry-base",
            "Base of exponential backoff",
            "base",
            RETRY_BASE,
            CLI::PositiveNumber);
        retryBaseOpt->needs(retryOpt);

        retryJitterOpt = addOption( //
            "--retry-jitter",
            "Maximum jitter in percent to apply randomly to calculated retry timeout (0 to disable)",
            "jitter",
            RETRY_JITTER,
            CLI::Range(0., 100.));
        retryJitterOpt->needs(retryOpt);

        retryLimitOpt = addOption( //
            "--retry-limit",
            "Upper limit in seconds of retry timeout (0 for infinite)",
            "sec",
            RETRY_LIMIT,
            CLI::NonNegativeNumber);
        retryLimitOpt->needs(retryOpt);
    }

    const std::map<int, const net::phy::PhysicalSocketOption>& ConfigPhysicalSocket::getSocketOptions() {
        return socketOptionsMap;
    }

    CLI::Option* ConfigPhysicalSocket::add_socket_option(const std::string& name,
                                                         int optLevel,
                                                         int optName,
                                                         const std::string& description,
                                                         const std::string& typeName,
                                                         const std::string& defaultValue,
                                                         const CLI::Validator& validator) {
        return net::config::ConfigSection::addFlagFunction(
            name,
            [this, strippedName = name.substr(0, name.find('{')), optLevel, optName](int64_t) -> void {
                try {
                    if (section->get_option(strippedName)->as<bool>()) {
                        socketOptionsMap.emplace(optName, net::phy::PhysicalSocketOption(optLevel, optName, 1));
                    } else {
                        socketOptionsMap.erase(optName);
                    }
                } catch (CLI::OptionNotFound& err) {
                    LOG(ERROR) << err.what();
                }
            },
            description,
            typeName,
            defaultValue,
            validator);
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, int optValue) {
        socketOptionsMap.emplace(optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue));

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::string& optValue) {
        socketOptionsMap.emplace(optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue));

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::vector<char>& optValue) {
        socketOptionsMap.emplace(optName, net::phy::PhysicalSocketOption(optLevel, optName, optValue));

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

        if (retry) {
            retryLimitOpt->remove_needs(retryOpt);
            retryJitterOpt->remove_needs(retryOpt);
            retryBaseOpt->remove_needs(retryOpt);
            retryTriesOpt->remove_needs(retryOpt);
            retryTimeoutOpt->remove_needs(retryOpt);
            retryOnFatalOpt->remove_needs(retryOpt);
        } else {
            retryLimitOpt->needs(retryOpt);
            retryJitterOpt->needs(retryOpt);
            retryBaseOpt->needs(retryOpt);
            retryTriesOpt->needs(retryOpt);
            retryTimeoutOpt->needs(retryOpt);
            retryOnFatalOpt->needs(retryOpt);
        }

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
