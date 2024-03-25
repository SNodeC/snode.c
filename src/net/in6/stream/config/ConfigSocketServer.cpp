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

#include "net/in6/stream/config/ConfigSocketServer.h"

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

namespace net::in6::stream::config {

    ConfigSocketServer::ConfigSocketServer(net::config::ConfigInstance* instance)
        : net::config::stream::ConfigSocketServer<net::in6::config::ConfigAddress, net::in6::config::ConfigAddressReverse>(instance) {
        net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>::setPortRequired();

        net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiFlags(AI_PASSIVE);
        net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiSockType(SOCK_STREAM);
        net::in6::config::ConfigAddress<net::config::ConfigAddressLocal>::setAiProtocol(IPPROTO_TCP);

        reusePortOpt = net::config::ConfigPhysicalSocket::add_socket_option( //
            "--reuse-port{true}",
            SOL_SOCKET,
            SO_REUSEPORT,
            "Reuse socket address",
            "bool",
            XSTR(REUSE_PORT),
            CLI::IsMember({"true", "false"}));

        iPv6OnlyOpt = net::config::ConfigPhysicalSocket::add_socket_option( //
            "--ipv6-only{true}",
            IPPROTO_IPV6,
            IPV6_V6ONLY,
            "Turn of IPv6 dual stack mode",
            "bool",
            XSTR(IPV6_ONLY),
            CLI::IsMember({"true", "false"}));
    }

    ConfigSocketServer::~ConfigSocketServer() {
    }

    ConfigSocketServer& ConfigSocketServer::setReusePort(bool reusePort) {
        if (reusePort) {
            addSocketOption(SOL_SOCKET, SO_REUSEPORT, 1);
        } else {
            removeSocketOption(SO_REUSEPORT);
        }

        reusePortOpt //
            ->default_val(reusePort ? "true" : "false")
            ->take_all()
            ->clear();

        return *this;
    }

    bool ConfigSocketServer::getReusePort() const {
        return reusePortOpt->as<bool>();
    }

    ConfigSocketServer& ConfigSocketServer::setIPv6Only(bool iPv6Only) {
        if (iPv6Only) {
            addSocketOption(IPPROTO_IPV6, IPV6_V6ONLY, 1);
        } else {
            removeSocketOption(IPV6_V6ONLY);
        }

        iPv6OnlyOpt //
            ->default_val(iPv6Only ? "true" : "false")
            ->take_all()
            ->clear();

        return *this;
    }

    bool ConfigSocketServer::getIPv6Only() const {
        return iPv6OnlyOpt->as<bool>();
    }

} // namespace net::in6::stream::config

template class net::config::stream::ConfigSocketServer<net::in6::config::ConfigAddress, net::in6::config::ConfigAddressReverse>;
