// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_STDIO_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/stdio/SocketReader.h"
#include "core/socket/stream/stdio/SocketWriter.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace core::socket::stream::stdio {

    /// Original stream connection engine composed with descriptor read/write hooks.
    template <typename PhysicalSocket, typename Config>
    class SocketConnection final : public core::socket::stream::SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config> {
    private:
        using Super = core::socket::stream::SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>;

    public:
        /// Construct only; call activate once and never use the pointer after false.
        SocketConnection(PhysicalSocket&& physicalSocket,
                         const std::function<void(SocketConnection*)>& onDisconnect,
                         std::uint64_t connectionId,
                         const std::shared_ptr<Config>& config);
        /// Attach the original factory/context after successful activation; original connection owns the result.
        using Super::setSocketContext;
        /// Inspect establishment on this loop turn; a close request immediately makes this false.
        bool isActive() const {
            return stdio::SocketReader::isEnabled() && stdio::SocketWriter::isEnabled();
        }
    };

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETCONNECTION_H
