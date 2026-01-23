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

#ifndef CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H
#define CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H

namespace core {
    namespace timer {
        class Timer;
    }

    namespace eventreceiver {
        class AcceptEventReceiver;
        class ConnectEventReceiver;
    } // namespace eventreceiver

    namespace socket::stream {
        class SocketContextFactory;
    }
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class AutoConnectControl : public std::enable_shared_from_this<AutoConnectControl> {
    public:
        AutoConnectControl();

        AutoConnectControl(const AutoConnectControl&) = delete;
        AutoConnectControl& operator=(const AutoConnectControl&) = delete;

        AutoConnectControl(AutoConnectControl&&) = delete;
        AutoConnectControl& operator=(AutoConnectControl&&) = delete;

        ~AutoConnectControl();

        void stopRetry();
        void stopReconnect();
        void stopAll();

        bool retryIsEnabled() const;
        bool reconnectIsEnabled() const;

        std::uint64_t getRetryGeneration() const;
        std::uint64_t getReconnectGeneration() const;

        bool isRetryGeneration(std::uint64_t generation) const;
        bool isReconnectGeneration(std::uint64_t generation) const;

    private:
        void armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher);
        void armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher);

        void scheduleCancelRetry();
        void scheduleCancelReconnect();

        void cancelRetryTimerInLoop();
        void cancelReconnectTimerInLoop();

        std::atomic<bool> retryEnabled{true};
        std::atomic<bool> reconnectEnabled{true};

        std::atomic<std::uint64_t> retryGeneration{1};
        std::atomic<std::uint64_t> reconnectGeneration{1};

        std::atomic<bool> cancelRetryScheduled{false};
        std::atomic<bool> cancelReconnectScheduled{false};

        std::unique_ptr<core::timer::Timer> retryTimer;
        std::unique_ptr<core::timer::Timer> reconnectTimer;

        template <typename SocketConnectorT, typename SocketContextFactoryT, typename... Args>
            requires std::is_base_of_v<core::eventreceiver::ConnectEventReceiver, SocketConnectorT> &&
                     std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactoryT>
        friend class SocketClient;

        template <typename SocketAcceptorT, typename SocketContextFactoryT, typename... Args>
            requires std::is_base_of_v<core::eventreceiver::AcceptEventReceiver, SocketAcceptorT> &&
                     std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactoryT>
        friend class SocketServer;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H
