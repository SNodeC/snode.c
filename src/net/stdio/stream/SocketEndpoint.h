// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef NET_STDIO_STREAM_SOCKETENDPOINT_H
#define NET_STDIO_STREAM_SOCKETENDPOINT_H
#include "core/socket/stream/stdio/SocketEndpoint.h"
#include "core/socket/stream/stdio/SocketEndpoint.hpp" // IWYU pragma: export
#include "net/stdio/phy/stream/PhysicalSocket.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
namespace net::stdio::stream {
    /// Stdio binding of the original stream factory; no protocol-specific endpoint interface.
    template <typename Factory, typename... Args>
    using SocketEndpoint = core::socket::stream::stdio::
        SocketEndpoint<net::stdio::phy::stream::PhysicalSocket, config::ConfigSocketEndpoint, Factory, Args...>;
} // namespace net::stdio::stream
#endif // NET_STDIO_STREAM_SOCKETENDPOINT_H
