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

#include "net/l2/stream/ConfigBind.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    ConfigBind::ConfigBind(CLI::App* baseSc) {
        bindSc = baseSc->add_subcommand("bind");
        bindSc->group("Sub-Options (use -h,--help on them)");
        bindSc->description("Server socket bind options");
        bindSc->configurable();

        bindHostOpt = bindSc->add_option("-a,--host", bindHost, "Bind bluetooth address");
        bindHostOpt->type_name("[bluetooth address]");
        bindHostOpt->default_val(":::::");
        bindHostOpt->configurable();

        bindPsmOpt = bindSc->add_option("-p,--psm", bindPsm, "Bind protocol service multiplexer");
        bindPsmOpt->type_name("[uint16_t]");
        bindPsmOpt->default_val(0);
        bindPsmOpt->configurable();
    }

    const std::string& ConfigBind::getBindHost() const {
        return bindHost;
    }

    uint16_t ConfigBind::getBindPsm() const {
        return bindPsm;
    }

    SocketAddress ConfigBind::getBindAddress() const {
        return net::l2::SocketAddress(bindHost, bindPsm);
    }

} // namespace net::l2::stream
