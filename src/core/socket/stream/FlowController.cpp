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

#include "core/timer/Timer.h"
#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    uint64_t FlowController::idCounter = 0;

    FlowController::FlowController(net::config::ConfigInstance* configInstance)
        : observedConfigInstance(configInstance)
        , onDestroy([](FlowController*) {
        })
        , onFlowRetryCallback([](FlowController*) {
        })
        , onFlowTerminatedCallback([](FlowController*) {
        })
        , onFlowStartedCallback([](FlowController*) {
        }) {
    }

    FlowController::~FlowController() {
        onDestroy(this);
    }

    std::string FlowController::getInstanceName() const {
        return observedConfigInstance->getInstanceName();
    }

    uint64_t FlowController::getId() const {
        return id;
    }

    bool FlowController::terminateFlow() {
        bool success = false;

        if (!terminated) {
            terminated = true;
            retryEnabled = false;
            cancelRetryTimer();

            terminateAsyncSubFlow();
            notifyFlowTerminated();

            success = true;
        }

        return success;
    }

    bool FlowController::isTerminated() const {
        return terminated;
    }

    void FlowController::stopRetry() {
        retryEnabled = false;
        cancelRetryTimer();
    }

    bool FlowController::isRetryEnabled() const {
        return retryEnabled;
    }

    FlowController* FlowController::setOnDestroy(const std::function<void(FlowController*)>& onDestroy) {
        observedConfigInstance->setOnDestroy([this, onDestroy](net::config::ConfigInstance*) {
            onDestroy(this);
        });

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

    FlowController* FlowController::onFlowCompleted(const std::function<void(const std::string& instanceName)>& callback) {
        observedConfigInstance->setOnDestroy(
            [instanceName = observedConfigInstance->getInstanceName(), callback](net::config::ConfigInstance*) {
                callback(instanceName);
            });

        return this;
    }

    FlowController* FlowController::onFlowCompleted(const std::function<void(uint64_t)>& callback) {
        observedConfigInstance->setOnDestroy([id = getId(), callback](net::config::ConfigInstance*) {
            callback(id);
        });

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

    void FlowController::reportFlowRetry() {
        onFlowRetryCallback(this);
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

} // namespace core::socket::stream
