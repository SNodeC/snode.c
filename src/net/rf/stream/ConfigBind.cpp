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

#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf::stream {

    ConfigBind::ConfigBind(CLI::App* baseSc) {
        bindSc = baseSc->add_subcommand("bind");
        bindSc->description("Bind options");
        bindSc->configurable();
        bindSc->required();

        bindHostOpt = bindSc->add_option("-a,--host", bindHost, "Bluetooth address");
        bindHostOpt->type_name("[bluetooth address]");
        bindHostOpt->default_val("00:00:00:00:00:00");
        bindHostOpt->configurable();

        bindChannelOpt = bindSc->add_option("-c,--channel", bindChannel, "Channel number");
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

    void ConfigBind::required(bool req) const {
        utils::Config::instance().required(bindSc, req);
        utils::Config::instance().required(bindChannelOpt, req);
    }

} // namespace net::rf::stream
