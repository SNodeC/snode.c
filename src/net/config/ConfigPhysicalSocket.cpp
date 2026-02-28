/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ConfigPhysicalSocket.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <functional>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::config {

    const std::map<int, std::map<int, net::phy::PhysicalSocketOption>>& ConfigPhysicalSocket::getSocketOptions() const {
        return socketOptionsMapMap;
    }

    CLI::Option* ConfigPhysicalSocket::addSocketOption(const std::string& name,
                                                       int optLevel,
                                                       int optName,
                                                       const std::string& description,
                                                       const std::string& typeName,
                                                       const std::string& defaultValue,
                                                       const CLI::Validator& validator) {
        return addFlagFunction(
                   name,
                   [this, strippedName = name.substr(0, name.find('{')), optLevel, optName]([[maybe_unused]] std::uint64_t) {
                       try {
                           try {
                               if (getOption(strippedName)->as<bool>()) {
                                   addSocketOption(optLevel, optName, getOption(strippedName)->as<bool>() ? 1 : 0);
                               } else {
                                   addSocketOption(optLevel, optName, 0);
                               }
                           } catch ([[maybe_unused]] CLI::ConversionError& err) {
                               removeSocketOption(optLevel, optName);
                           }
                       } catch (CLI::OptionNotFound& err) {
                           LOG(ERROR) << err.what();
                       }
                   },
                   description,
                   typeName,
                   defaultValue,
                   validator)
            ->force_callback();
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, int optValue) {
        socketOptionsMapMap[optLevel][optName] = net::phy::PhysicalSocketOption(optLevel, optName, optValue);

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::string& optValue) {
        socketOptionsMapMap[optLevel][optName] = net::phy::PhysicalSocketOption(optLevel, optName, optValue);

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::addSocketOption(int optLevel, int optName, const std::vector<char>& optValue) {
        socketOptionsMapMap[optLevel][optName] = net::phy::PhysicalSocketOption(optLevel, optName, optValue);

        return *this;
    }

    ConfigPhysicalSocket& ConfigPhysicalSocket::removeSocketOption(int optLevel, int optName) {
        socketOptionsMapMap[optLevel].erase(optName);
        if (socketOptionsMapMap[optLevel].empty()) {
            socketOptionsMapMap.erase(optLevel);
        }

        return *this;
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

    const std::string ConfigPhysicalSocket::retry = XSTR(RETRY);
    const std::string ConfigPhysicalSocket::retryOnFatal = XSTR(RETRY_ON_FATAL);
    float ConfigPhysicalSocket::retryTimeout = RETRY_TIMEOUT;
    uint16_t ConfigPhysicalSocket::retryTries = RETRY_TRIES;
    double ConfigPhysicalSocket::retryBase = RETRY_BASE;
    float ConfigPhysicalSocket::retryJitter = RETRY_JITTER;
    float ConfigPhysicalSocket::retryLimit = RETRY_LIMIT;

} // namespace net::config
