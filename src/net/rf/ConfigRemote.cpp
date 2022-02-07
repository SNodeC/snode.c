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

#include "net/rf/ConfigRemote.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf {

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

        connectChannelOpt = connectSc->add_option("-c,--channel", connectChannel, "Channel number");
        connectChannelOpt->type_name("[uint8_t]");
        connectChannelOpt->default_val(0);
        connectChannelOpt->take_first();
        connectChannelOpt->configurable();
    }

    SocketAddress ConfigRemote::getAddress() const {
        return SocketAddress(connectHost, connectChannel);
    }

    void ConfigRemote::required() const {
        connectSc->required();
        connectHostOpt->required();
        connectChannelOpt->required();
    }

    bool ConfigRemote::isPresent() const {
        return connectHostOpt->count() > 0 || connectChannelOpt->count() > 0;
    }

    void ConfigRemote::updateFromCommandLine() {
        if (connectHostOpt->count() > 0) {
            remoteAddress.setAddress(connectHost);
        }
        if (connectChannelOpt->count() > 0) {
            remoteAddress.setChannel(connectChannel);
        }
    }

} // namespace net::rf
