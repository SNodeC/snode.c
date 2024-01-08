/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "net/config/stream/ConfigSocketServer.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "core/system/netdb.h"

#include <memory>
#include <netinet/in.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::in::stream::config {

    ConfigSocketServer::ConfigSocketServer(net::config::ConfigInstance* instance)
        : net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress, net::in::config::ConfigAddressReverse>(instance) {
        net::in::config::ConfigAddress<net::config::ConfigAddressLocal>::setPortRequired();
        net::in::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiFlags(AI_PASSIVE);
        net::in::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiSockType(SOCK_STREAM);
        net::in::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiProtocol(IPPROTO_TCP);

        net::config::ConfigPhysicalSocket::add_socket_option(reusePortOpt, //
                                                             "--reuse-port",
                                                             SOL_SOCKET,
                                                             SO_REUSEPORT,
                                                             "Reuse port number",
                                                             "bool",
                                                             XSTR(REUSE_PORT),
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

    bool ConfigSocketServer::getReusePort() const {
        return reusePortOpt->as<bool>();
    }

} // namespace net::in::stream::config

template class net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress, net::in::config::ConfigAddressReverse>;
