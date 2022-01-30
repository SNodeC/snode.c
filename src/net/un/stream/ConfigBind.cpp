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

#include "net/un/stream/ConfigBind.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream {
    ConfigBind::ConfigBind(CLI::App* baseSc) {
        bindSc = baseSc->add_subcommand("bind");
        bindSc->group("Sub-Options (use -h,--help on them)");
        bindSc->description("Server socket bind options");
        bindSc->configurable();

        bindSunPathOpt = bindSc->add_option("-p,--path", bindSunPath, "Unix domain socket path");
        bindSunPathOpt->type_name("[filesystem path]");
        bindSunPathOpt->default_val("/tmp/sun.sock");
        bindSunPathOpt->configurable();
    }

    const std::string& ConfigBind::getBindSunPath() const {
        return bindSunPath;
    }

    SocketAddress ConfigBind::getBindAddress() const {
        return net::un::SocketAddress(bindSunPath);
    }

} // namespace net::un::stream
