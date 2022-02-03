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

#include "net/l2/ConfigRemote.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2 {

    ConfigRemote::ConfigRemote(CLI::App* baseSc) {
        connectSc = baseSc->add_subcommand("remote");
        connectSc->description("Connect options");
        connectSc->configurable();
        connectSc->required();

        connectHostOpt = connectSc->add_option("-a,--host", connectHost, "Bluetooth address");
        connectHostOpt->type_name("[bluetooth address]");
        connectHostOpt->default_val("00:00:00:00:00:00");
        connectHostOpt->take_first();
        connectHostOpt->configurable();

        connectPsmOpt = connectSc->add_option("-p,--psm", connectPsm, "Protocol service multiplexer");
        connectPsmOpt->type_name("[uint16_t]");
        connectPsmOpt->default_val(0);
        connectPsmOpt->take_first();
        connectPsmOpt->configurable();
    }

    SocketAddress ConfigRemote::getRemoteAddress() const {
        utils::Config::instance().parse(true);

        return SocketAddress(connectHost, connectPsm);
    }

    void ConfigRemote::required() const {
        connectSc->required();
        connectHostOpt->required();
        connectPsmOpt->required();
    }

} // namespace net::l2
