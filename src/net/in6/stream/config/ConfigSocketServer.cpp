/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "net/in6/stream/config/ConfigSocketServer.h"

#include "net/config/stream/ConfigSocketServer.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wmissing-noreturn")
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
#endif
#endif
#endif
#include "utils/CLI11.hpp"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "core/system/netdb.h"

#include <fstream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>

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

        reuseAddressOpt = net::config::ConfigPhysicalSocket::addSocketOption( //
            "--reuse-address{true}",
            SOL_SOCKET,
            SO_REUSEADDR,
            "Reuse socket address",
            "bool",
            XSTR(IN6_REUSE_ADDRESS),
            CLI::IsMember({"true", "false"}));

        reusePortOpt = net::config::ConfigPhysicalSocket::addSocketOption( //
            "--reuse-port{true}",
            SOL_SOCKET,
            SO_REUSEPORT,
            "Reuse port number",
            "bool",
            XSTR(IN6_REUSE_PORT),
            CLI::IsMember({"true", "false"}));

        std::string ipv6Only = XSTR(IN6_IPV6_ONLY);
        if (ipv6Only == "default") {
            std::ifstream in("/proc/sys/net/ipv6/bindv6only");
            if (in) {
                std::string s;
                in >> s; // reads "0" or "1"

                ipv6Only = s == "1" ? "true" : "false";
            }
            in.close();
        }
        iPv6OnlyOpt = net::config::ConfigPhysicalSocket::addSocketOption( //
            "--ipv6-only{true}",
            IPPROTO_IPV6,
            IPV6_V6ONLY,
            "Turn off IPv6 dual stack mode",
            "bool",
            XSTR(IN6_IPV6_ONLY),
            CLI::IsMember({"true", "false", "default"}));
        if (std::string(XSTR(IN6_IPV6_ONLY)) == "default") {
            iPv6OnlyOpt->default_str(ipv6Only);
        }

        disableNagleAlgorithmOpt = net::config::ConfigPhysicalSocket::addSocketOption( //
            "--disable-nagle-algorithm{true}",
            IPPROTO_TCP,
            TCP_NODELAY,
            "Turn of Nagle algorithm",
            "tristat",
            XSTR(IN6_SERVER_DISABLE_NAGLE_ALGORITHM),
            CLI::IsMember({"true", "false", "default"}));
        if (std::string(XSTR(IN6_SERVER_DISABLE_NAGLE_ALGORITHM)) == "default") {
            disableNagleAlgorithmOpt->default_str("false");
        }
    }

    ConfigSocketServer::~ConfigSocketServer() {
    }

    ConfigSocketServer& ConfigSocketServer::setReuseAddress(bool reuseAddress) {
        if (reuseAddress) {
            addSocketOption(SOL_SOCKET, SO_REUSEADDR, 1);
        } else {
            addSocketOption(SOL_SOCKET, SO_REUSEADDR, 0);
        }

        reuseAddressOpt //
            ->default_val(reuseAddress ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigSocketServer::getReuseAddress() const {
        return reuseAddressOpt->as<bool>();
    }

    ConfigSocketServer& ConfigSocketServer::setReusePort(bool reusePort) {
        if (reusePort) {
            addSocketOption(SOL_SOCKET, SO_REUSEPORT, 1);
        } else {
            addSocketOption(SOL_SOCKET, SO_REUSEPORT, 0);
        }

        reusePortOpt //
            ->default_val(reusePort ? "true" : "false")
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
            addSocketOption(IPPROTO_IPV6, IPV6_V6ONLY, 0);
        }

        iPv6OnlyOpt //
            ->default_val(iPv6Only ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigSocketServer::getIPv6Only() const {
        return iPv6OnlyOpt->as<bool>();
    }

    ConfigSocketServer& ConfigSocketServer::setDisableNagleAlgorithm(bool disableNagleAlgorithm) {
        if (disableNagleAlgorithm) {
            addSocketOption(IPPROTO_TCP, TCP_NODELAY, 1);
        } else {
            addSocketOption(IPPROTO_TCP, TCP_NODELAY, 0);
        }

        disableNagleAlgorithmOpt //
            ->default_val(disableNagleAlgorithm ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigSocketServer::getDisableNagleAlgorithm() const {
        return disableNagleAlgorithmOpt->as<bool>();
    }

} // namespace net::in6::stream::config

template class net::config::stream::ConfigSocketServer<net::in6::config::ConfigAddress, net::in6::config::ConfigAddressReverse>;
