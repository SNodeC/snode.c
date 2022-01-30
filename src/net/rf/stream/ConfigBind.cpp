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

    void ConfigBind::populate(CLI::App* serverSc) {
        serverBindSc = serverSc->add_subcommand("bind");
        serverBindSc->group("Sub-Options (use -h,--help on them)");
        serverBindSc->description("Server socket bind options");
        serverBindSc->configurable();

        bindServerHostOpt = serverBindSc->add_option("-a,--host", bindInterface, "Bind bluetooth address");
        bindServerHostOpt->type_name("[bluetooth address]");
        bindServerHostOpt->default_val(":::::");
        bindServerHostOpt->configurable();

        bindServerChannelOpt = serverBindSc->add_option("-c,--channel", channel, "Bind channel number");
        bindServerChannelOpt->type_name("[uint8_t]");
        bindServerChannelOpt->default_val(0);
        bindServerChannelOpt->configurable();
    }

    const std::string& ConfigBind::getBindInterface() const {
        return bindInterface;
    }

    uint8_t ConfigBind::getChannel() const {
        return channel;
    }

    SocketAddress ConfigBind::getBindAddress() const {
        return net::rf::SocketAddress(bindInterface, channel);
    }

} // namespace net::rf::stream
