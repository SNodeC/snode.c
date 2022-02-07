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

#include "net/un/ConfigRemote.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un {

    ConfigRemote::ConfigRemote(CLI::App* baseSc) {
        connectSc = baseSc->add_subcommand("remote");
        connectSc->description("Connect options");
        connectSc->configurable();

        connectSunPathOpt = connectSc->add_option("-p,--path", connectSunPath, "Unix domain socket");
        connectSunPathOpt->type_name("[filesystem path]");
        connectSunPathOpt->default_val("/tmp/sun.sock");
        connectSunPathOpt->take_first();
        connectSunPathOpt->configurable();
    }

    SocketAddress ConfigRemote::getAddress() const {
        return SocketAddress(connectSunPath);
    }

    void ConfigRemote::required() const {
        connectSc->required();
        connectSunPathOpt->required();
    }

    bool ConfigRemote::isPresent() const {
        return connectSunPathOpt->count() > 0;
    }

} // namespace net::un
