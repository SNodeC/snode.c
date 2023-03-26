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

#include "net/config/ConfigListen.h"

#include "net/config/ConfigInstance.h"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DEFAULT_ACCEPTSPERTICK
#define DEFAULT_ACCEPTSPERTICK 1
#endif

#ifndef DEFAULT_BACKLOG
#define DEFAULT_BACKLOG 5
#endif

namespace net::config {

    ConfigListen::ConfigListen(ConfigInstance* instance)
        : net::config::ConfigSection(
              instance, "server", "Configuration of server socket for instance '" + instance->getInstanceName() + "'") {
        add_option(backlogOpt, //
                   "--backlog",
                   "Listen backlog",
                   "backlog",
                   DEFAULT_BACKLOG,
                   CLI::PositiveNumber);
        add_option(acceptsPerTickOpt, //
                   "--accepts-per-tick",
                   "Accepts per tick",
                   "number",
                   DEFAULT_ACCEPTSPERTICK,
                   CLI::PositiveNumber);
    }

    int ConfigListen::getBacklog() const {
        return backlogOpt->as<int>();
    }

    void ConfigListen::setBacklog(int newBacklog) {
        backlogOpt //
            ->default_val(newBacklog)
            ->clear();
    }

    int ConfigListen::getAcceptsPerTick() const {
        return acceptsPerTickOpt->as<int>();
    }

    void ConfigListen::setAcceptsPerTick(int acceptsPerTickSet) {
        acceptsPerTickOpt //
            ->default_val(acceptsPerTickSet)
            ->clear();
    }

} // namespace net::config
