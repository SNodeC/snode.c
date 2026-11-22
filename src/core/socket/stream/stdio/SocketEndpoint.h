// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_H
#define CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_H

#include "core/socket/Socket.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/socket/stream/stdio/SocketConnection.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace core::socket::stream::stdio {

    /// Configurable, copyable attachment facade; use and callbacks belong to the event-loop thread.
    /// Status callbacks must not throw; TAKE requires exclusive use of the supplied descriptor numbers.
    template <typename PhysicalSocket, typename ConfigT, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    class SocketEndpoint : public core::socket::Socket<ConfigT> {
        using Super = core::socket::Socket<ConfigT>;

    public:
        using Config = ConfigT;
        using SocketConnection = stdio::SocketConnection<PhysicalSocket, Config>;
        using SocketAddress = typename PhysicalSocket::SocketAddress;
        using StatusCallback = std::function<void(const SocketAddress&, core::socket::State)>;
        /// Construct configuration/factory only; queued attempts retain their shared state independently.
        explicit SocketEndpoint(const std::string& name, Args&&... args);
        /// Queue resolution of the configured binding; never acquires descriptors synchronously.
        void attach(const StatusCallback& onStatus) const;
        /// Queue a snapshot binding; reports one status per executed attempt, without retries.
        void attach(const SocketAddress& address, const StatusCallback& onStatus) const;
        /// Append a callback before factory attachment; initialize replaces the callback chain.
        const SocketEndpoint& setOnConnect(const std::function<void(SocketConnection*)>& callback, bool initialize = false) const;
        /// Append a callback after activation; closing here cancels establishment safely.
        const SocketEndpoint& setOnConnected(const std::function<void(SocketConnection*)>& callback, bool initialize = false) const;
        /// Append a teardown callback for established connections only; no facade pointer is retained.
        const SocketEndpoint& setOnDisconnect(const std::function<void(SocketConnection*)>& callback, bool initialize = false) const;
        /// Return shared ownership of the original stream factory.
        std::shared_ptr<Factory> getSocketContextFactory() const;

    private:
        struct Context {
            std::shared_ptr<Factory> factory;
            std::function<void(SocketConnection*)> onConnect, onConnected, onDisconnect;
            std::uint64_t connections = 0;
        };
        void queue(const std::optional<SocketAddress>& address, const StatusCallback& onStatus) const;
        std::shared_ptr<Context> sharedContext;
    };
} // namespace core::socket::stream::stdio
#endif // CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_H
