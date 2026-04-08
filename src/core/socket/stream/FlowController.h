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

namespace core::timer {
    class Timer;
}

namespace net::config {
    class ConfigInstance; // IWYU pragma: export
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class FlowController {
    public:
        FlowController(net::config::ConfigInstance* configInstance);

        FlowController(const FlowController&) = delete;
        FlowController& operator=(const FlowController&) = delete;

        FlowController(FlowController&&) = delete;
        FlowController& operator=(FlowController&&) = delete;

        virtual ~FlowController();

        std::string getInstanceName() const;

        uint64_t getId() const;

        bool terminateFlow();
        bool isTerminated() const;

        void stopRetry();
        bool isRetryEnabled() const;

        FlowController* setOnDestroy(const std::function<void(FlowController*)>& onDestroy);

        FlowController* onFlowRetry(const std::function<void(FlowController*)>& callback);
        FlowController* onFlowCompleted(const std::function<void(const std::string&)>& callback);
        FlowController* onFlowCompleted(const std::function<void(uint64_t)>& callback);
        FlowController* onFlowTerminated(const std::function<void(FlowController*)>& callback);
        FlowController* onFlowStarted(const std::function<void(FlowController*)>& callback);

    protected:
        void reportFlowRetry();
        void reportFlowStarted();

        void armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher);

        virtual void terminateAsyncSubFlow() = 0;

    private:
        uint64_t id{idCounter++};
        static uint64_t idCounter;
        void cancelRetryTimer();
        void notifyFlowTerminated();

        bool retryEnabled{true};
        bool terminated{false};

        net::config::ConfigInstance* observedConfigInstance;

        std::unique_ptr<core::timer::Timer> retryTimer;

        std::function<void(FlowController*)> onDestroy;

        std::function<void(FlowController*)> onFlowRetryCallback;
        std::function<void(FlowController*)> onFlowTerminatedCallback;
        std::function<void(FlowController*)> onFlowStartedCallback;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_FLOWCONTROLLER_H
