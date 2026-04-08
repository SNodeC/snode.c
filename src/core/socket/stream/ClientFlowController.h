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

#ifndef CORE_SOCKET_STREAM_CLIENTFLOWCONTROLLER_H
#define CORE_SOCKET_STREAM_CLIENTFLOWCONTROLLER_H

#include "core/socket/stream/FlowController.h" // IWYU pragma: export
#include "core/timer/Timer.h"

namespace core {
    namespace socket::stream {
        class SocketContextFactory;
    }
    namespace eventreceiver {
        class ConnectEventReceiver;
    } // namespace eventreceiver
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <type_traits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class ClientFlowController : public FlowController {
    public:
        ClientFlowController(net::config::ConfigInstance* configInstance);

        void stopReconnect();
        bool isReconnectEnabled() const;

        ClientFlowController* onFlowReconnect(const std::function<void(ClientFlowController*)>& callback);

    private:
        void reportFlowReconnect();

        void observeConnectEventReceiver(core::eventreceiver::ConnectEventReceiver* connectEventReceiver);

        void armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher);

        void terminateAsyncSubFlow() override;

        void cancelReconnectTimer();

        bool reconnectEnabled{true};

        core::eventreceiver::ConnectEventReceiver* connectEventReceiver{nullptr};

        std::unique_ptr<core::timer::Timer> reconnectTimer;

        std::function<void(ClientFlowController*)> onFlowReconnectCallback;

        template <typename SocketConnectorT, typename SocketContextFactoryT, typename... Args>
            requires std::is_base_of_v<core::eventreceiver::ConnectEventReceiver, SocketConnectorT> &&
                     std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactoryT>
        friend class SocketClient;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_CLIENTFLOWCONTROLLER_H
