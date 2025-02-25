/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
#define CORE_SOCKET_STREAM_SOCKETSERVERNEW_H

#include "core/SNodeC.h"
#include "core/socket/Socket.h" // IWYU pragma: export
#include "core/socket/State.h"  // IWYU pragma: export
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Random.h"

#include <algorithm>
#include <functional> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketAcceptorT, typename SocketContextFactoryT, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactoryT>
    class SocketServer : public core::socket::Socket<typename SocketAcceptorT::Config> {
    private:
        using SocketAcceptor = SocketAcceptorT;
        using SocketContextFactory = SocketContextFactoryT;

        using Super = core::socket::Socket<typename SocketAcceptor::Config>;

    public:
        using SocketConnection = typename SocketAcceptor::SocketConnection;
        using SocketAddress = typename SocketAcceptor::SocketAddress;

        SocketServer(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>(std::forward<Args>(args)...))
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

        SocketServer(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : SocketServer("", onConnect, onConnected, onDisconnect, std::forward<Args>(args)...) {
        }

        // VLOG() is used hire as this are log messages for the application
        SocketServer(const std::string& name, Args&&... args)
            : SocketServer(
                  name,
                  [name](SocketConnection* socketConnection) { // onConnect
                      VLOG(2) << name << ": OnConnect";

                      VLOG(2) << "  Local: " << socketConnection->getLocalAddress().toString();
                      VLOG(2) << "   Peer: " << socketConnection->getRemoteAddress().toString();
                  },
                  [name](SocketConnection* socketConnection) { // onConnected
                      VLOG(2) << name << ": OnConnected";

                      VLOG(2) << "  Local: " << socketConnection->getLocalAddress().toString();
                      VLOG(2) << "   Peer: " << socketConnection->getRemoteAddress().toString();
                  },
                  [name](SocketConnection* socketConnection) { // onDisconnect
                      VLOG(2) << name << ": OnDisconnect";

                      VLOG(2) << "  Local: " << socketConnection->getLocalAddress().toString();
                      VLOG(2) << "   Peer: " << socketConnection->getRemoteAddress().toString();
                  },
                  std::forward<Args>(args)...) {
        }

        explicit SocketServer(Args&&... args)
            : SocketServer("", std::forward<Args>(args)...) {
        }

    private:
        void realListen(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                        unsigned int tries,
                        double retryTimeoutScale) const {
            if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                new SocketAcceptor(
                    socketContextFactory,
                    onConnect,
                    onConnected,
                    onDisconnect,
                    [server = *this, onStatus, tries, retryTimeoutScale](const SocketAddress& socketAddress,
                                                                         core::socket::State state) mutable {
                        const bool retry = (state & core::socket::State::NO_RETRY) == 0 &&
                                           (server.getConfig().getRetryTries() == 0 || tries < server.getConfig().getRetryTries());

                        state &= ~core::socket::State::NO_RETRY;
                        onStatus(socketAddress, state);

                        if (retry) {
                            switch (state) {
                                case core::socket::State::OK:
                                    server.currentOk++;
                                    [[fallthrough]];
                                case core::socket::State::DISABLED:
                                    break;
                                case core::socket::State::ERROR:
                                    server.currentError++;
                                    break;
                                case core::socket::State::FATAL:
                                    server.currentFatal++;
                                    break;
                            }

                            if ((server.currentError > 0 && server.getConfig().getRetry()) ||
                                (server.currentFatal > 0 && server.getConfig().getRetry() && server.getConfig().getRetryOnFatal())) {
                                if (server.totalOk < server.currentError + server.currentFatal) {
                                    server.totalOk += server.currentOk;

                                    double relativeRetryTimeout =
                                        server.getConfig().getRetryLimit() > 0
                                            ? std::min<double>(server.getConfig().getRetryTimeout() * retryTimeoutScale,
                                                               server.getConfig().getRetryLimit())
                                            : server.getConfig().getRetryTimeout() * retryTimeoutScale;
                                    relativeRetryTimeout -= utils::Random::getInRange(-server.getConfig().getRetryJitter(),
                                                                                      server.getConfig().getRetryJitter()) *
                                                            relativeRetryTimeout / 100.;

                                    LOG(INFO) << server.getConfig().getInstanceName() << ": OnStatus";
                                    LOG(INFO) << " retrying in " << relativeRetryTimeout << " seconds";

                                    core::timer::Timer::singleshotTimer(
                                        [server, onStatus, tries, retryTimeoutScale]() mutable {
                                            server.getConfig().Local::renew();
                                            server.currentOk = 0;
                                            server.currentError = 0;
                                            server.currentFatal = 0;

                                            server.realListen(onStatus, tries + 1, retryTimeoutScale * server.getConfig().getRetryBase());
                                        },
                                        relativeRetryTimeout);
                                }
                            }
                        }
                    },
                    Super::config);
            }
        }

    public:
        void listen(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            realListen(onStatus, 0, 1);
        }

        void listen(const SocketAddress& localAddress,
                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setSocketAddress(localAddress);

            listen(onStatus);
        }

        void listen(const SocketAddress& localAddress,
                    int backlog,
                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setBacklog(backlog);

            listen(localAddress, onStatus);
        }

        std::function<void(SocketConnection*)> setOnConnect(const std::function<void(SocketConnection*)>& onConnect) {
            std::function<void(SocketConnection*)> oldOnConnect = this->onConnect;

            this->onConnect = onConnect;

            return oldOnConnect;
        }

        std::function<void(SocketConnection*)> setOnConnected(const std::function<void(SocketConnection*)>& onConnected) {
            std::function<void(SocketConnection*)> oldOnConnected = this->onConnected;

            this->onConnected = onConnected;

            return oldOnConnected;
        }

        std::function<void(SocketConnection*)> setOnDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            std::function<void(SocketConnection*)> oldOnDisconnect = this->onDisconnect;

            this->onDisconnect = onDisconnect;

            return oldOnDisconnect;
        }

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() {
            return socketContextFactory;
        }

    private:
        std::shared_ptr<SocketContextFactory> socketContextFactory;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

        std::size_t totalOk = 0;
        std::size_t currentOk = 0;
        std::size_t currentError = 0;
        std::size_t currentFatal = 0;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETSERVERNEW_H
