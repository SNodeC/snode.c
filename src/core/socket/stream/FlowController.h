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

#ifndef CORE_SOCKET_STREAM_FLOWCONTROLLER_H
#define CORE_SOCKET_STREAM_FLOWCONTROLLER_H

namespace core {
    namespace timer {
        class Timer;
    }

    namespace eventreceiver {
        class AcceptEventReceiver;
        class ConnectEventReceiver;
    } // namespace eventreceiver
} // namespace core

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class FlowController {
    public:
        FlowController();

        FlowController(const FlowController&) = delete;
        FlowController& operator=(const FlowController&) = delete;

        FlowController(FlowController&&) = delete;
        FlowController& operator=(FlowController&&) = delete;

        virtual ~FlowController();

        bool terminateFlow();

        void stopRetry();
        bool isRetryEnabled() const;

        FlowController* setOnDestroy(const std::function<void(FlowController*)>& onDestroy);

        FlowController* onFlowRetry(const std::function<void(FlowController*)>& callback);
        FlowController* onFlowCompleted(const std::function<void(FlowController*)>& callback);
        FlowController* onFlowTerminated(const std::function<void(FlowController*)>& callback);
        FlowController* onFlowStarted(const std::function<void(FlowController*)>& callback);

        void observeConfig(net::config::ConfigInstance* configInstance);

        void reportFlowRetry();
        void reportFlowCompleted();
        void reportFlowStarted();

        void armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher);

    protected:
        virtual void terminateAsyncSubFlow() = 0;

    private:
        void cancelRetryTimer();
        void notifyFlowTerminated();

        bool retryEnabled{true};
        bool terminated{false};
        bool flowCompletedNotified{false};
        net::config::ConfigInstance* observedConfigInstance{nullptr};

        std::unique_ptr<core::timer::Timer> retryTimer;

        std::function<void(FlowController*)> onDestroy;

        std::function<void(FlowController*)> onFlowRetryCallback;
        std::function<void(FlowController*)> onFlowCompletedCallback;
        std::function<void(FlowController*)> onFlowTerminatedCallback;
        std::function<void(FlowController*)> onFlowStartedCallback;
    };

    class ClientFlowController : public FlowController {
    public:
        ClientFlowController();

        void stopReconnect();
        bool isReconnectEnabled() const;

        ClientFlowController* onFlowReconnect(const std::function<void(ClientFlowController*)>& callback);

        void reportFlowReconnect();

        void observeConnectEventReceiver(core::eventreceiver::ConnectEventReceiver* connectEventReceiver);

        void armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher);

    protected:
        void terminateAsyncSubFlow() override;

    private:
        void cancelReconnectTimer();

        bool reconnectEnabled{true};

        core::eventreceiver::ConnectEventReceiver* connectEventReceiver{nullptr};

        std::unique_ptr<core::timer::Timer> reconnectTimer;

        std::function<void(ClientFlowController*)> onFlowReconnectCallback;
    };

    class ServerFlowController : public FlowController {
    public:
        void observeAcceptEventReceiver(core::eventreceiver::AcceptEventReceiver* acceptEventReceiver);

    protected:
        void terminateAsyncSubFlow() override;

    private:
        core::eventreceiver::AcceptEventReceiver* acceptEventReceiver{nullptr};
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_FLOWCONTROLLER_H
