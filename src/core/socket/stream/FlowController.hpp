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

#include "core/EventReceiver.h"
#include "core/socket/stream/FlowController.h"
#include "core/timer/Timer.h"
#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename ConcreteFlowController>
    uint64_t FlowController<ConcreteFlowController>::idCounter = 0;

    template <typename ConcreteFlowController>
    FlowController<ConcreteFlowController>::FlowController(net::config::ConfigInstance* configInstance)
        : observedConfigInstance(configInstance)
        , onFlowRetryCallback([](FlowController*) {
        })
        , onFlowTerminatedCallback([](FlowController*) {
        }) {
    }

    template <typename ConcreteFlowController>
    FlowController<ConcreteFlowController>::~FlowController() = default;

    template <typename ConcreteFlowController>
    std::string FlowController<ConcreteFlowController>::getInstanceName() const {
        return observedConfigInstance->getInstanceName();
    }

    template <typename ConcreteFlowController>
    uint64_t FlowController<ConcreteFlowController>::getId() const {
        return id;
    }

    template <typename ConcreteFlowController>
    bool FlowController<ConcreteFlowController>::terminateFlow() {
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

    template <typename ConcreteFlowController>
    bool FlowController<ConcreteFlowController>::isTerminated() const {
        return terminated;
    }

    template <typename ConcreteFlowController>
    void FlowController<ConcreteFlowController>::stopRetry() {
        retryEnabled = false;
        cancelRetryTimer();
    }

    template <typename ConcreteFlowController>
    bool FlowController<ConcreteFlowController>::isRetryEnabled() const {
        return retryEnabled;
    }

    template <typename ConcreteFlowController>
    ConcreteFlowController*
    FlowController<ConcreteFlowController>::setOnFlowRetry(const std::function<void(ConcreteFlowController*)>& callback) {
        const std::function<void(ConcreteFlowController*)> oldCallback = onFlowRetryCallback;
        onFlowRetryCallback = [oldCallback, callback](ConcreteFlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return dynamic_cast<ConcreteFlowController*>(this);
    }

    template <typename ConcreteFlowController>
    ConcreteFlowController*
    FlowController<ConcreteFlowController>::setOnFlowCompleted(const std::function<void(uint64_t, const std::string&)>& callback) {
        observedConfigInstance->setOnDestroy([callback, id = getId()](net::config::ConfigInstance* configInstance) {
            callback(id, configInstance->getInstanceName());
        });

        return dynamic_cast<ConcreteFlowController*>(this);
    }

    template <typename ConcreteFlowController>
    ConcreteFlowController*
    FlowController<ConcreteFlowController>::setOnFlowTerminated(const std::function<void(ConcreteFlowController*)>& callback) {
        const std::function<void(ConcreteFlowController*)> oldCallback = onFlowTerminatedCallback;
        onFlowTerminatedCallback = [oldCallback, callback](ConcreteFlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return dynamic_cast<ConcreteFlowController*>(this);
    }

    template <typename ConcreteFlowController>
    void FlowController<ConcreteFlowController>::startFlow(const std::function<void()>& callback) {
        core::EventReceiver::atNextTick([this, callback] {
            if (!terminated) {
                callback();
            }
        });
    }

    template <typename ConcreteFlowController>
    void FlowController<ConcreteFlowController>::reportFlowRetry() {
        onFlowRetryCallback(dynamic_cast<ConcreteFlowController*>(this));
    }

    template <typename ConcreteFlowController>
    void FlowController<ConcreteFlowController>::armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        if (retryEnabled) {
            retryTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    template <typename ConcreteFlowController>
    void FlowController<ConcreteFlowController>::cancelRetryTimer() {
        if (retryTimer) {
            retryTimer->cancel();
            retryTimer.reset();
        }
    }

    template <typename ConcreteFlowController>
    void FlowController<ConcreteFlowController>::notifyFlowTerminated() {
        onFlowTerminatedCallback(dynamic_cast<ConcreteFlowController*>(this));
    }

} // namespace core::socket::stream
