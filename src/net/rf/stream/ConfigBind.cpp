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

#include "net/rf/stream/ConfigBind.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf::stream {

    ConfigBind::ConfigBind(CLI::App* baseSc) {
        bindSc = baseSc->add_subcommand("bind");
        bindSc->group("Sub-Options (use -h,--help on them)");
        bindSc->description("Server socket bind options");
        bindSc->configurable();

        bindHostOpt = bindSc->add_option("-a,--host", bindHost, "Bind bluetooth address");
        bindHostOpt->type_name("[bluetooth address]");
        bindHostOpt->default_val(":::::");
        bindHostOpt->configurable();

        bindChannelOpt = bindSc->add_option("-c,--channel", bindChannel, "Bind channel number");
        bindChannelOpt->type_name("[uint8_t]");
        bindChannelOpt->default_val(0);
        bindChannelOpt->configurable();
    }

    const std::string& ConfigBind::getBindHost() const {
        return bindHost;
    }

    uint8_t ConfigBind::getBindChannel() const {
        return bindChannel;
    }

    SocketAddress ConfigBind::getBindAddress() const {
        return net::rf::SocketAddress(bindHost, bindChannel);
    }

} // namespace net::rf::stream
