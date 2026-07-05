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

#ifndef CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_SOCKETACCEPTOR_H

#include "core/eventreceiver/AcceptEventReceiver.h" // IWYU pragma: export
#include "core/socket/State.h"
#include "log/LogScopeOwner.h"
#include "log/SemanticLogger.h"

namespace core::socket::stream {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <optional>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    /** Sequence diagram showing how a connect to a peer is performed.
    @startuml
    !include core/socket/stream/pu/SocketAcceptor.pu
    @enduml
    */
    template <typename PhysicalSocketServerT,
              typename ConfigT,
              template <typename PhysicalSocketServer, typename Config> typename SocketConnectionT>
    class SocketAcceptor : protected core::eventreceiver::AcceptEventReceiver {
    private:
        using PhysicalServerSocket = PhysicalSocketServerT;

    protected:
        using Config = ConfigT;
        using SocketAddress = typename PhysicalServerSocket::SocketAddress;
        using SocketConnection = SocketConnectionT<PhysicalServerSocket, Config>;

    public:
        SocketAcceptor(const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(core::eventreceiver::AcceptEventReceiver*)>& onInitState,
                       const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                       const std::shared_ptr<Config>& config);

        SocketAcceptor(const SocketAcceptor& socketAcceptor);

        ~SocketAcceptor() override;

        virtual void init();

        logger::BoundaryLogger log() const {
            // Transitional Round 5 API shape only.
            // Backend-backed default semantic sinks are introduced in Round 6.
            // Until then, the no-argument overload returns a no-op logger;
            // tests and early validation must use the sink-taking overload.
            return log([](logger::LogRecord) {});
        }

        logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
                                   logger::LogLevel threshold = logger::LogLevel::Trace,
                                   logger::BoundaryLogger::Clock clock = {}) const {
            return logScope.logger(std::move(sink), threshold, std::move(clock));
        }

    protected:
        virtual void useNextSocketAddress() = 0;

    private:
        void acceptEvent() final;

        void unobservedEvent() final;

    protected:
        void destruct() final;

    private:
        PhysicalServerSocket physicalServerSocket;
        SocketAddress configuredAddress;

    protected:
        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;
        std::function<void(core::eventreceiver::AcceptEventReceiver*)> onInitState;

        std::function<void(const SocketAddress&, core::socket::State)> onStatus = nullptr;


        static logger::LogScopeOwner makeLogScope(const std::string& instanceName) {
            return logger::LogScopeOwner(logger::LogOrigin::Framework,
                                         logger::LogBoundary::Instance,
                                         "core.socket.stream",
                                         instanceName.empty() ? std::nullopt : std::optional<std::string>(instanceName),
                                         logger::LogRole::Server,
                                         std::nullopt);
        }

        logger::LogScopeOwner logScope;
        std::shared_ptr<Config> config;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
