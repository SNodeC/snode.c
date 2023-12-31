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

#include "net/config/ConfigInstance.h"
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
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::in::stream::config {

    ConfigSocketServer::ConfigSocketServer(net::config::ConfigInstance* instance)
        : net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress>(instance) {
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

        CLI::App* app = instance->add_section("remote", "Remote side of connection for instance '" + instance->getInstanceName() + "'");
        numericReverseOpt = app->add_flag("--numeric-reverse", "Suppress reverse host name lookup")
                                ->type_name("bool")
                                ->default_val(XSTR(IPV4_NUMERIC_REVERSE))
                                ->take_last()
                                ->group(app->get_formatter()->get_label("Persistent Options"))
                                ->check(CLI::IsMember({"true", "false"}));
    }

    ConfigSocketServer::~ConfigSocketServer() {
    }

    SocketAddress ConfigSocketServer::newSocketAddress(const SocketAddress::SockAddr& sockAddr, SocketAddress::SockLen sockAddrLen) {
        return SocketAddress(sockAddr, sockAddrLen, numericReverseOpt->as<bool>());
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

    ConfigSocketServer& ConfigSocketServer::setNumericReverse(bool numeric) {
        numericReverseOpt //
            ->default_val(numeric)
            ->clear();

        return *this;
    }

    bool ConfigSocketServer::getNumericReverse() const {
        return numericReverseOpt->as<bool>();
    }

} // namespace net::in::stream::config

template class net::config::stream::ConfigSocketServer<net::in::config::ConfigAddress>;
