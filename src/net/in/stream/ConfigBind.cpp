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

#include "net/in/stream/ConfigBind.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    ConfigBind::ConfigBind(CLI::App* serverSc) {
        serverBindSc = serverSc->add_subcommand("bind");
        serverBindSc->group("Sub-Options (use -h,--help on them)");
        serverBindSc->description("Server socket bind options");
        serverBindSc->configurable();

        bindServerHostOpt = serverBindSc->add_option("-a,--host", bindInterface, "Bind host name or IP address");
        bindServerHostOpt->type_name("[hostname|ip]");
        bindServerHostOpt->default_val("0.0.0.0");
        bindServerHostOpt->configurable();

        bindServerPortOpt = serverBindSc->add_option("-p,--port", bindPort, "Bind port number");
        bindServerPortOpt->type_name("[uint16_t]");
        bindServerPortOpt->default_val(0);
        bindServerPortOpt->configurable();
    }

    const std::string& ConfigBind::getBindInterface() const {
        return bindInterface;
    }

    uint16_t ConfigBind::getBindPort() const {
        return bindPort;
    }

    SocketAddress ConfigBind::getBindAddress() const {
        return net::in::SocketAddress(bindInterface, bindPort);
    }

} // namespace net::in::stream
