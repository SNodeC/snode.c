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

#ifndef APPS_MODEL_LOWLEVELLEGACYCLIENT_H
#define APPS_MODEL_LOWLEVELLEGACYCLIENT_H

#include "log/Logger.h"

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define SOCKETSERVER_INCLUDE QUOTE_INCLUDE(net/NET/stream/STREAM/SocketServer.h)
// clang-format on

#include SOCKETSERVER_INCLUDE // IWYU pragma: export

#include "EchoSocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::echo::model::legacy {

    using EchoSocketServer = net::NET::stream::legacy::SocketServer<EchoServerSocketContextFactory>;

    EchoSocketServer getServer() {
        return EchoSocketServer("echoserver");
    }

} // namespace apps::echo::model::legacy

#elif (STREAM_TYPE == TLS) // tls

namespace apps::echo::model::tls {

    using EchoSocketServer = net::NET::stream::tls::SocketServer<EchoServerSocketContextFactory>;

    EchoSocketServer getServer() {
        return EchoSocketServer("echoserver");
    }

} // namespace apps::echo::model::tls

#endif

#endif // APPS_MODEL_LOWLEVELLEGACYCLIENT_H
