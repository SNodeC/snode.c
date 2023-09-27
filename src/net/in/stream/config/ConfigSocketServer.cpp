/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "net/in/stream/config/ConfigSocketServer.h"

#include "net/stream/config/ConfigSocketServer.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "core/system/netdb.h"

#include <memory>
#include <netinet/in.h>
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream::config {

    ConfigSocketServer::ConfigSocketServer(net::config::ConfigInstance* instance)
        : net::stream::config::ConfigSocketServer<net::in::config::ConfigAddress>(instance) {
        net::in::config::ConfigAddress<net::config::ConfigAddressLocal>::portRequired();
        net::in::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiFlags(AI_PASSIVE)
            .setAiSocktype(SOCK_STREAM)
            .setAiProtocol(IPPROTO_TCP);

        net::config::ConfigPhysicalSocket::add_socket_option(reusePortOpt, //
                                                             "--reuse-port",
                                                             SOL_SOCKET,
                                                             SO_REUSEPORT,
                                                             "Reuse port number",
                                                             "bool",
                                                             SNODEC_DEFAULT_REUSE_PORT,
                                                             CLI::IsMember({"true", "false"}));
    }

    ConfigSocketServer::~ConfigSocketServer() {
    }

    void ConfigSocketServer::setReusePort(bool reusePort) {
        if (reusePort) {
            addSocketOption(SOL_SOCKET, SO_REUSEPORT, 1);
        } else {
            removeSocketOption(SO_REUSEPORT);
        }

        reusePortOpt //
            ->default_val(reusePort ? "true" : "false")
            ->take_all()
            ->clear();
    }

    bool ConfigSocketServer::getReusePort() {
        return reusePortOpt->as<bool>();
    }

} // namespace net::in::stream::config

template class net::stream::config::ConfigSocketServer<net::in::config::ConfigAddress>;
