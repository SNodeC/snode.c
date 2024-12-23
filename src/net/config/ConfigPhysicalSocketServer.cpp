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

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigPhysicalSocketServer::ConfigPhysicalSocketServer(ConfigInstance* instance)
        : Super(instance) {
        backlogOpt = addOption( //
            "--backlog",
            "Listen backlog",
            "backlog",
            BACKLOG,
            CLI::PositiveNumber);

        acceptsPerTickOpt = addOption( //
            "--accepts-per-tick",
            "Accepts per tick",
            "number",
            ACCEPTS_PER_TICK,
            CLI::PositiveNumber);

        acceptTimeoutOpt = addOption( //
            "--accept-timeout",
            "Accept timeout",
            "timeout",
            ACCEPT_TIMEOUT,
            CLI::NonNegativeNumber);
    }

    int ConfigPhysicalSocketServer::getBacklog() const {
        return backlogOpt->as<int>();
    }

    ConfigPhysicalSocketServer& ConfigPhysicalSocketServer::setBacklog(int newBacklog) {
        backlogOpt //
            ->default_val(newBacklog)
            ->clear();

        return *this;
    }

    int ConfigPhysicalSocketServer::getAcceptsPerTick() const {
        return acceptsPerTickOpt->as<int>();
    }

    ConfigPhysicalSocketServer& ConfigPhysicalSocketServer::setAcceptsPerTick(int acceptsPerTickSet) {
        acceptsPerTickOpt //
            ->default_val(acceptsPerTickSet)
            ->clear();

        return *this;
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
