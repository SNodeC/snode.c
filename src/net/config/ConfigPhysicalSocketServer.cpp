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

#include "ConfigPhysicalSocketServer.h"

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

    ConfigPhysicalSocketServer::ConfigPhysicalSocketServer(ConfigInstance* instance)
        : net::config::ConfigPhysicalSocket(instance) {
        acceptTimeoutOpt = add_option( //
            "--accept-timeout",
            "Accept timeout",
            "timeout",
            ACCEPT_TIMEOUT,
            CLI::NonNegativeNumber);
    }

    ConfigPhysicalSocketServer& ConfigPhysicalSocketServer::setAcceptTimeout(const utils::Timeval& acceptTimeout) {
        acceptTimeoutOpt //
            ->default_val(acceptTimeout)
            ->clear();

        return *this;
    }

    utils::Timeval ConfigPhysicalSocketServer::getAcceptTimeout() const {
        return acceptTimeoutOpt->as<utils::Timeval>();
    }

} // namespace net::config
