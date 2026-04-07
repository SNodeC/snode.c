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

#include "core/socket/stream/FlowController.h"

#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/timer/Timer.h"
#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    FlowController::FlowController()
        : onDestroy([](FlowController*) {
        })
        , onFlowRetryCallback([](FlowController*) {
        })
        , onFlowCompletedCallback([](FlowController*) {
        })
        , onFlowTerminatedCallback([](FlowController*) {
        })
        , onFlowStartedCallback([](FlowController*) {
        }) {
    }

    FlowController::~FlowController() {
        onDestroy(this);
    }

    bool FlowController::terminateFlow() {
        if (terminated) {
            return false;
        }

        terminated = true;
        retryEnabled = false;
        cancelRetryTimer();

        terminateAsyncSubFlow();
        notifyFlowTerminated();

        return true;
    }

    void FlowController::stopRetry() {
        retryEnabled = false;
        cancelRetryTimer();
    }

    bool FlowController::isRetryEnabled() const {
        return retryEnabled;
    }

    FlowController* FlowController::setOnDestroy(const std::function<void(FlowController*)>& onDestroy) {
        const std::function<void(FlowController*)> oldOnDestroy = this->onDestroy;

        this->onDestroy = [oldOnDestroy, onDestroy](FlowController* control) {
            oldOnDestroy(control);
            onDestroy(control);
        };
        return this;
    }

    FlowController* FlowController::onFlowRetry(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowRetryCallback;
        onFlowRetryCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    FlowController* FlowController::onFlowCompleted(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowCompletedCallback;
        onFlowCompletedCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    FlowController* FlowController::onFlowTerminated(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowTerminatedCallback;
        onFlowTerminatedCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    FlowController* FlowController::onFlowStarted(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowStartedCallback;
        onFlowStartedCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    void FlowController::observeConfig(net::config::ConfigInstance* configInstance) {
        if (configInstance == nullptr || configInstance == observedConfigInstance) {
            return;
        }

        observedConfigInstance = configInstance;

        configInstance->setOnDestroy([this](net::config::ConfigInstance*) {
            reportFlowCompleted();
        });
    }

    void FlowController::reportFlowRetry() {
        onFlowRetryCallback(this);
    }

    void FlowController::reportFlowCompleted() {
        if (!flowCompletedNotified) {
            flowCompletedNotified = true;
            onFlowCompletedCallback(this);
        }
    }

    void FlowController::reportFlowStarted() {
        onFlowStartedCallback(this);
    }

    void FlowController::armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        if (retryEnabled) {
            retryTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    void FlowController::cancelRetryTimer() {
        if (retryTimer) {
            retryTimer->cancel();
            retryTimer.reset();
        }
    }

    void FlowController::notifyFlowTerminated() {
        onFlowTerminatedCallback(this);
    }

    ClientFlowController::ClientFlowController()
        : onFlowReconnectCallback([](ClientFlowController*) {
        }) {
    }

    void ClientFlowController::stopReconnect() {
        reconnectEnabled = false;
        cancelReconnectTimer();
    }

    bool ClientFlowController::isReconnectEnabled() const {
        return reconnectEnabled;
    }

    ClientFlowController* ClientFlowController::onFlowReconnect(const std::function<void(ClientFlowController*)>& callback) {
        const std::function<void(ClientFlowController*)> oldCallback = onFlowReconnectCallback;
        onFlowReconnectCallback = [oldCallback, callback](ClientFlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    void ClientFlowController::reportFlowReconnect() {
        onFlowReconnectCallback(this);
    }

    void ClientFlowController::observeConnectEventReceiver(core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
        if (connectEventReceiver != nullptr && connectEventReceiver->isEnabled()) {
            this->connectEventReceiver = connectEventReceiver;
        } else {
            this->connectEventReceiver = nullptr;
        }
    }

    void ClientFlowController::armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        if (reconnectEnabled) {
            reconnectTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    void ClientFlowController::terminateAsyncSubFlow() {
        stopReconnect();
        stopRetry();

        if (connectEventReceiver != nullptr) {
            connectEventReceiver->stopConnect();
        }
    }

    void ClientFlowController::cancelReconnectTimer() {
        if (reconnectTimer) {
            reconnectTimer->cancel();
            reconnectTimer.reset();
        }
    }

    void ServerFlowController::observeAcceptEventReceiver(core::eventreceiver::AcceptEventReceiver* acceptEventReceiver) {
        if (acceptEventReceiver != nullptr && acceptEventReceiver->isEnabled()) {
            this->acceptEventReceiver = acceptEventReceiver;
        } else {
            this->acceptEventReceiver = nullptr;
        }
    }

    void ServerFlowController::terminateAsyncSubFlow() {
        stopRetry();

        if (acceptEventReceiver != nullptr) {
            acceptEventReceiver->stopListen();
        }
    }

} // namespace core::socket::stream
