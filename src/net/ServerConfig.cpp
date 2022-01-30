/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "net/ServerConfig.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    ServerConfig::ServerConfig() {
    }

    void ServerConfig::populate(CLI::App* serverSc) {
        serverBacklogOpt = serverSc->add_option("-b,--backlog", backlog, "Server listen backlog");
        serverBacklogOpt->type_name("[backlog]");
        serverBacklogOpt->default_val(5);
        serverBacklogOpt->configurable();
    }

    int ServerConfig::getBacklog() const {
        return backlog;
    }

} // namespace net
