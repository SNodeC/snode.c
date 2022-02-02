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

#include "net/rf/ConfigLocal.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf {

    ConfigLocal::ConfigLocal(CLI::App* baseSc) {
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

    SocketAddress ConfigLocal::getLocalAddress() const {
        return SocketAddress(bindHost, bindChannel);
    }

    void ConfigLocal::required() const {
        bindSc->required();
        bindChannelOpt->required();
    }

} // namespace net::rf
