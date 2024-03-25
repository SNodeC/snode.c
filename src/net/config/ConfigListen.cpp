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

#include "net/config/ConfigListen.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigListen::ConfigListen(ConfigInstance* instance)
        : net::config::ConfigSection(instance, "server", "Configuration of server socket") {
        backlogOpt = add_option( //
            "--backlog",
            "Listen backlog",
            "backlog",
            BACKLOG,
            CLI::PositiveNumber);
        acceptsPerTickOpt = add_option( //
            "--accepts-per-tick",
            "Accepts per tick",
            "number",
            ACCEPTS_PER_TICK,
            CLI::PositiveNumber);
    }

    int ConfigListen::getBacklog() const {
        return backlogOpt->as<int>();
    }

    ConfigListen& ConfigListen::setBacklog(int newBacklog) {
        backlogOpt //
            ->default_val(newBacklog)
            ->clear();

        return *this;
    }

    int ConfigListen::getAcceptsPerTick() const {
        return acceptsPerTickOpt->as<int>();
    }

    ConfigListen& ConfigListen::setAcceptsPerTick(int acceptsPerTickSet) {
        acceptsPerTickOpt //
            ->default_val(acceptsPerTickSet)
            ->clear();

        return *this;
    }

} // namespace net::config
