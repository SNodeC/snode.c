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

#include "core/socket/stream/AutoConnectControl.h"

#include "core/EventReceiver.h"
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    AutoConnectControl::AutoConnectControl() = default;

    AutoConnectControl::~AutoConnectControl() = default;

    void AutoConnectControl::stopRetry() {
        retryEnabled = false;
        ++retryGeneration;
        scheduleCancelRetry();
    }

    void AutoConnectControl::stopReconnect() {
        reconnectEnabled = false;
        ++reconnectGeneration;
        scheduleCancelReconnect();
    }

    void AutoConnectControl::stopAll() {
        retryEnabled = false;
        reconnectEnabled = false;

        ++retryGeneration;
        ++reconnectGeneration;

        scheduleCancelRetry();
        scheduleCancelReconnect();
    }

    bool AutoConnectControl::retryIsEnabled() const {
        return retryEnabled;
    }

    bool AutoConnectControl::reconnectIsEnabled() const {
        return reconnectEnabled;
    }

    std::uint64_t AutoConnectControl::getRetryGeneration() const {
        return retryGeneration;
    }

    std::uint64_t AutoConnectControl::getReconnectGeneration() const {
        return reconnectGeneration;
    }

    bool AutoConnectControl::isRetryGeneration(std::uint64_t generation) const {
        return retryGeneration == generation;
    }

    bool AutoConnectControl::isReconnectGeneration(std::uint64_t generation) const {
        return reconnectGeneration == generation;
    }

    void AutoConnectControl::armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        cancelRetryTimerInLoop();
        if (retryEnabled) {
            retryTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    void AutoConnectControl::armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        cancelReconnectTimerInLoop();
        if (reconnectEnabled) {
            reconnectTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    void AutoConnectControl::scheduleCancelRetry() {
        if (cancelRetryScheduled) {
            return;
        }
        cancelRetryScheduled = true;
        core::EventReceiver::atNextTick([self = shared_from_this()] {
            self->cancelRetryTimerInLoop();
            self->cancelRetryScheduled = false;
        });
    }

    void AutoConnectControl::scheduleCancelReconnect() {
        if (cancelReconnectScheduled) {
            return;
        }
        cancelReconnectScheduled = true;
        core::EventReceiver::atNextTick([self = shared_from_this()] {
            self->cancelReconnectTimerInLoop();
            self->cancelReconnectScheduled = false;
        });
    }

    void AutoConnectControl::cancelRetryTimerInLoop() {
        if (retryTimer) {
            retryTimer->cancel();
            retryTimer.reset();
        }
    }

    void AutoConnectControl::cancelReconnectTimerInLoop() {
        if (reconnectTimer) {
            reconnectTimer->cancel();
            reconnectTimer.reset();
        }
    }

} // namespace core::socket::stream
