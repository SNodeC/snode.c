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

#include "ConfigPhysicalSocketClient.h"

#include "net/config/ConfigSection.hpp"
#include "utils/ResetToDefault.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstdint>
#include <memory>
#include <stdexcept>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigPhysicalSocketClient::ConfigPhysicalSocketClient(ConfigInstance* instance)
        : net::config::ConfigPhysicalSocket(instance) {
        Super::add_flag_function(
            reconnectOpt, //
            "--reconnect",
            [this](int64_t) -> void {
                if (!this->reconnectOpt->as<bool>()) {
                    this->reconnectTimeOpt->clear();
                }
                utils::ResetToDefault(this->reconnectOpt)(this->reconnectOpt->as<bool>());
            },
            "Automatically retry listen|connect",
            "bool",
            RECONNECT,
            CLI::IsMember({"true", "false"}));

        Super::add_option(reconnectTimeOpt, //
                          "--reconnect-time",
                          "Duration after disconnect bevore reconnect",
                          "sec",
                          RECONNECT_TIME,
                          CLI::NonNegativeNumber);
        reconnectTimeOpt->needs(reconnectOpt);

        Super::add_option(connectTimeoutOpt, //
                          "--connect-timeout",
                          "Connect timeout",
                          "timeout",
                          CONNECT_TIMEOUT,
                          CLI::NonNegativeNumber);
    }

    bool ConfigPhysicalSocketClient::getReconnect() const {
        return reconnectOpt->as<bool>();
    }

    void ConfigPhysicalSocketClient::setReconnect(bool reconnect) {
        reconnectOpt //
            ->default_val(reconnect)
            ->clear();
    }

    double ConfigPhysicalSocketClient::getReconnectTime() const {
        return reconnectTimeOpt->as<double>();
    }

    void ConfigPhysicalSocketClient::setReconnectTime(double time) {
        reconnectTimeOpt //
            ->default_val(time)
            ->clear();
    }

    void ConfigPhysicalSocketClient::setConnectTimeout(const utils::Timeval& connectTimeout) {
        connectTimeoutOpt //
            ->default_val(connectTimeout)
            ->clear();
    }

    utils::Timeval ConfigPhysicalSocketClient::getConnectTimeout() const {
        double connectTimeout = connectTimeoutOpt->as<double>();
        return utils::Timeval(connectTimeout);
    }

} // namespace net::config
