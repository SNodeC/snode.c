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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <memory>

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
        Super::add_option(connectTimeoutOpt, //
                          "--connect-timeout",
                          "Connect timeout",
                          "timeout",
                          CONNECT_TIMEOUT,
                          CLI::NonNegativeNumber);
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
