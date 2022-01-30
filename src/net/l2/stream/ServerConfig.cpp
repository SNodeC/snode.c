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

#include "net/l2/stream/ServerConfig.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    ServerConfig::ServerConfig(const std::string& name)
        : net::ConfigBase(name) {
        serverBindSc = serverSc->add_subcommand("bind");
        serverBindSc->group("Sub-Options (use -h,--help on them)");
        serverBindSc->description("Server socket bind options");
        serverBindSc->configurable();

        bindServerHostOpt = serverBindSc->add_option("-a,--host", bindInterface, "Bind bluetooth address");
        bindServerHostOpt->type_name("[bluetooth address]");
        bindServerHostOpt->default_val(":::::");
        bindServerHostOpt->configurable();

        bindServerPsmOpt = serverBindSc->add_option("-p,--psm", psm, "Bind protocol service multiplexer");
        bindServerPsmOpt->type_name("[uint16_t]");
        bindServerPsmOpt->default_val(0);
        bindServerPsmOpt->configurable();

        net::ConfigServer::populate(serverSc);
        net::ConfigConn::populate(serverSc);
    }

    const std::string& ServerConfig::getBindInterface() const {
        return bindInterface;
    }

    uint16_t ServerConfig::getPsm() const {
        return psm;
    }

    SocketAddress ServerConfig::getBindAddress() const {
        return net::l2::SocketAddress(bindInterface, psm);
    }

    void ServerConfig::required(bool req) const {
        utils::Config::instance().required(serverSc, req);
        utils::Config::instance().required(serverBindSc, req);
        utils::Config::instance().required(bindServerHostOpt, req);
        utils::Config::instance().required(bindServerPsmOpt, req);
    }

} // namespace net::l2::stream
