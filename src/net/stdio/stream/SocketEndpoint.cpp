// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
#include "net/stdio/stream/SocketEndpoint.h"

#include "core/socket/Socket.h"                        // IWYU pragma: keep
#include "core/socket/stream/SocketConnection.h"       // IWYU pragma: keep
#include "core/socket/stream/stdio/SocketConnection.h" // IWYU pragma: keep
template class core::socket::Socket<net::stdio::stream::config::ConfigSocketEndpoint>;
template class core::socket::stream::stdio::SocketConnection<net::stdio::phy::stream::PhysicalSocket,
                                                             net::stdio::stream::config::ConfigSocketEndpoint>;
