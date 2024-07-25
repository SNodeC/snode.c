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

#include "ConfigPhysicalSocketClient.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstdint>
#include <functional>

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

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::config {

    ConfigPhysicalSocketClient::ConfigPhysicalSocketClient(ConfigInstance* instance)
        : net::config::ConfigPhysicalSocket(instance) {
        reconnectOpt = addFlagFunction( //
            "--reconnect{true}",
            [this](int64_t) -> void {
                if (!this->reconnectOpt->as<bool>()) {
                    this->reconnectTimeOpt->clear();
                }
            },
            "Auto-reconnection in the event of a connection interruption",
            "bool",
            XSTR(RECONNECT),
            CLI::IsMember({"true", "false"}));

        reconnectTimeOpt = addOption( //
            "--reconnect-time",
            "Duration after disconnect before reconnect",
            "sec",
            RECONNECT_TIME,
            CLI::NonNegativeNumber);
        reconnectTimeOpt->needs(reconnectOpt);

        connectTimeoutOpt = addOption( //
            "--connect-timeout",
            "Connect timeout",
            "timeout",
            CONNECT_TIMEOUT,
            CLI::NonNegativeNumber);
    }

    ConfigPhysicalSocketClient& ConfigPhysicalSocketClient::setReconnect(bool reconnect) {
        reconnectOpt //
            ->default_val(reconnect ? "true" : "false")
            ->clear();

        if (reconnect) {
            reconnectTimeOpt->remove_needs(reconnectOpt);
        } else {
            reconnectTimeOpt->needs(reconnectOpt);
        }

        return *this;
    }

    bool ConfigPhysicalSocketClient::getReconnect() const {
        return reconnectOpt->as<bool>();
    }

    ConfigPhysicalSocketClient& ConfigPhysicalSocketClient::setReconnectTime(double time) {
        reconnectTimeOpt //
            ->default_val(time)
            ->clear();

        return *this;
    }

    double ConfigPhysicalSocketClient::getReconnectTime() const {
        return reconnectTimeOpt->as<double>();
    }

    ConfigPhysicalSocketClient& ConfigPhysicalSocketClient::setConnectTimeout(const utils::Timeval& connectTimeout) {
        connectTimeoutOpt //
            ->default_val(connectTimeout)
            ->clear();

        return *this;
    }

    utils::Timeval ConfigPhysicalSocketClient::getConnectTimeout() const {
        const double connectTimeout = connectTimeoutOpt->as<double>();
        return utils::Timeval(connectTimeout);
    }

} // namespace net::config
