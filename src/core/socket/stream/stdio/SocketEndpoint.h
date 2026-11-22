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

#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_H
#define CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_H

#include "core/socket/Socket.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/socket/stream/stdio/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

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
        /// Append teardown paired with onConnect, including later establishment failure; no facade pointer is retained.
        const SocketEndpoint& setOnDisconnect(const std::function<void(SocketConnection*)>& callback, bool initialize = false) const;
        /// Return shared ownership of the original stream factory.
        std::shared_ptr<Factory> getSocketContextFactory() const;

    private:
        struct Context {
            std::shared_ptr<Factory> factory;
            std::function<void(SocketConnection*)> onConnect;
            std::function<void(SocketConnection*)> onConnected;
            std::function<void(SocketConnection*)> onDisconnect;
            std::uint64_t connections = 0;
        };

        static void setCallback(std::function<void(SocketConnection*)>& current,
                                const std::function<void(SocketConnection*)>& callback,
                                bool initialize);

        void queue(const std::optional<SocketAddress>& address, const StatusCallback& onStatus) const;

        std::shared_ptr<Context> sharedContext;
    };

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_H
