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

#include "net/in6/stream/ConfigBind.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream {

    ConfigBind::ConfigBind(CLI::App* baseSc) {
        bindSc = baseSc->add_subcommand("bind");
        bindSc->description("Bind options");
        bindSc->configurable();
        bindSc->required();

        bindHostOpt = bindSc->add_option("-a,--host", bindHost, "Host name or IP address");
        bindHostOpt->type_name("[hostname|ip]");
        bindHostOpt->default_val("::");
        bindHostOpt->configurable();

        bindPortOpt = bindSc->add_option("-p,--port", bindPort, "Port number");
        bindPortOpt->type_name("[uint16_t]");
        bindPortOpt->default_val(0);
        bindPortOpt->configurable();
    }

    const std::string& ConfigBind::getBindHost() const {
        return bindHost;
    }

    uint16_t ConfigBind::getBindPort() const {
        return bindPort;
    }

    SocketAddress ConfigBind::getBindAddress() const {
        return SocketAddress(bindHost, bindPort);
    }

    void ConfigBind::required() const {
        bindSc->required();
        bindPortOpt->required();
    }

} // namespace net::in6::stream
