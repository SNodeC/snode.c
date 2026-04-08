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

#include "core/EventReceiver.h"
#include "core/timer/Timer.h"
#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    namespace detail {
        inline std::atomic_uint64_t flowControllerIdCounter = 0;
    }

    template <typename DerivedT>
    class FlowController : public std::enable_shared_from_this<DerivedT> {
    public:
        using Derived = DerivedT;

        explicit FlowController(net::config::ConfigInstance* configInstance)
            : observedConfigInstance(configInstance)
            , id(detail::flowControllerIdCounter.fetch_add(1, std::memory_order_relaxed))
            , onDestroy([](Derived*) {
            })
            , onFlowRetryCallback([](Derived*) {
            })
            , onFlowTerminatedCallback([](Derived*) {
            })
            , onFlowStartedCallback([](Derived*) {
            }) {
        }

        FlowController(const FlowController&) = delete;
        FlowController& operator=(const FlowController&) = delete;

        FlowController(FlowController&&) = delete;
        FlowController& operator=(FlowController&&) = delete;

        virtual ~FlowController() {
            onDestroy(self());
        }

        std::string getInstanceName() const {
            return observedConfigInstance->getInstanceName();
        }

        uint64_t getId() const {
            return id;
        }

        void startFlow(const std::function<void()>& callback) {
            std::weak_ptr<Derived> selfWeak = this->shared_from_this();
            core::EventReceiver::atNextTick([selfWeak, callback]() {
                if (const std::shared_ptr<Derived> selfLocked = selfWeak.lock(); selfLocked != nullptr && !selfLocked->isTerminated()) {
                    selfLocked->reportFlowStarted();
                    callback();
                }
            });
        }

        bool terminateFlow() {
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

        bool isTerminated() const {
            return terminated;
        }

        void stopRetry() {
            retryEnabled = false;
            cancelRetryTimer();
        }

        bool isRetryEnabled() const {
            return retryEnabled;
        }

        Derived* setOnDestroy(const std::function<void(Derived*)>& onDestroySet) {
            observedConfigInstance->setOnDestroy([this, onDestroySet](net::config::ConfigInstance*) {
                onDestroySet(self());
            });

            return self();
        }

        Derived* onFlowRetry(const std::function<void(Derived*)>& callback) {
            const std::function<void(Derived*)> oldCallback = onFlowRetryCallback;
            onFlowRetryCallback = [oldCallback, callback](Derived* flowController) {
                oldCallback(flowController);
                callback(flowController);
            };

            return self();
        }

        Derived* onFlowCompleted(const std::function<void(const std::string&)>& callback) {
            observedConfigInstance->setOnDestroy(
                [instanceName = observedConfigInstance->getInstanceName(), callback](net::config::ConfigInstance*) {
                    callback(instanceName);
                });

            return self();
        }

        Derived* onFlowCompleted(const std::function<void(uint64_t)>& callback) {
            observedConfigInstance->setOnDestroy([id = getId(), callback](net::config::ConfigInstance*) {
                callback(id);
            });

            return self();
        }

        Derived* onFlowTerminated(const std::function<void(Derived*)>& callback) {
            const std::function<void(Derived*)> oldCallback = onFlowTerminatedCallback;
            onFlowTerminatedCallback = [oldCallback, callback](Derived* flowController) {
                oldCallback(flowController);
                callback(flowController);
            };

            return self();
        }

        Derived* onFlowStarted(const std::function<void(Derived*)>& callback) {
            const std::function<void(Derived*)> oldCallback = onFlowStartedCallback;
            onFlowStartedCallback = [oldCallback, callback](Derived* flowController) {
                oldCallback(flowController);
                callback(flowController);
            };

            return self();
        }

    protected:
        void reportFlowRetry() {
            onFlowRetryCallback(self());
        }

        void reportFlowStarted() {
            onFlowStartedCallback(self());
        }

        void armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
            if (retryEnabled) {
                retryTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
            }
        }

        virtual void terminateAsyncSubFlow() = 0;

    private:
        Derived* self() {
            return static_cast<Derived*>(this);
        }

        void cancelRetryTimer() {
            if (retryTimer) {
                retryTimer->cancel();
                retryTimer.reset();
            }
        }

        void notifyFlowTerminated() {
            onFlowTerminatedCallback(self());
        }

        net::config::ConfigInstance* observedConfigInstance;
        uint64_t id;

        bool retryEnabled{true};
        bool terminated{false};

        std::unique_ptr<core::timer::Timer> retryTimer;

        std::function<void(Derived*)> onDestroy;

        std::function<void(Derived*)> onFlowRetryCallback;
        std::function<void(Derived*)> onFlowTerminatedCallback;
        std::function<void(Derived*)> onFlowStartedCallback;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_FLOWCONTROLLER_H
