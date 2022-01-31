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

#include "net/in/stream/ConfigConnect.h"

#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    ConfigConnect::ConfigConnect(CLI::App* baseSc) {
        connectSc = baseSc->add_subcommand("connect");
        connectSc->description("Connect options");
        connectSc->configurable();

        connectHostOpt = connectSc->add_option("-a,--host", connectHost, "Host name or IP address");
        connectHostOpt->type_name("[hostname|ip]");
        connectHostOpt->default_val("0.0.0.0");
        connectHostOpt->configurable();

        connectPortOpt = connectSc->add_option("-p,--port", connectPort, "Port number");
        connectPortOpt->type_name("[uint16_t]");
        connectPortOpt->default_val(0);
        connectPortOpt->configurable();
    }

    const std::string& ConfigConnect::getConnectHost() const {
        return connectHost;
    }

    uint16_t ConfigConnect::getConnectPort() const {
        return connectPort;
    }

    SocketAddress ConfigConnect::getConnectAddress() const {
        return SocketAddress(connectHost, connectPort);
    }

    void ConfigConnect::required(bool req) const {
        utils::Config::instance().required(connectSc, req);
        utils::Config::instance().required(connectHostOpt, req);
        utils::Config::instance().required(connectPortOpt, req);
    }

} // namespace net::in::stream
