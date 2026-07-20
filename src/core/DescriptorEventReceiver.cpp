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

#include "core/DescriptorEventReceiver.h"

#include "core/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <climits>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    Observer::~Observer() {
    }

    void Observer::observed() {
        observationCounter++;
    }

    void Observer::unObserved() {
        observationCounter--;

        if (observationCounter == 0) {
            unobservedEvent();
        }
    }

    const utils::Timeval DescriptorEventReceiver::TIMEOUT::DEFAULT = {-1, 0};
    const utils::Timeval DescriptorEventReceiver::TIMEOUT::DISABLE = {0, 0};
    const utils::Timeval DescriptorEventReceiver::TIMEOUT::MAX = {LONG_MAX, 0};

    DescriptorEventReceiver::DescriptorEventReceiver(const std::string& name,
                                                     DescriptorEventPublisher& descriptorEventPublisher,
                                                     const utils::Timeval& timeout)
        : EventReceiver(name)
        , descriptorEventPublisher(descriptorEventPublisher)
        , logScope(logger::LogOrigin::Framework, logger::LogBoundary::System, "core.eventreceiver", name)
        , maxInactivity(timeout)
        , initialTimeout(timeout) {
    }

    int DescriptorEventReceiver::getRegisteredFd() const {
        return observedFd;
    }

    logger::BoundaryLogger DescriptorEventReceiver::log() const {
        return logScope.logger(logger::Logger::semanticSink());
    }

    logger::BoundaryLogger
    DescriptorEventReceiver::log(logger::BoundaryLogger::Sink sink, logger::LogLevel threshold, logger::BoundaryLogger::Clock clock) const {
        return logScope.logger(std::move(sink), threshold, std::move(clock));
    }

    bool DescriptorEventReceiver::isEnabled() const {
        return enabled;
    }

    bool DescriptorEventReceiver::isSuspended() const {
        return suspended;
    }

    bool DescriptorEventReceiver::enable(int fd) {
        if (enabled) {
            log().warn("{}: Double enable", getName());
            return false;
        }

        observedFd = fd;
        if (descriptorEventPublisher.enable(this)) {
            enabled = true;
            log().trace("{}: Enabled", getName());
            return true;
        }

        const int registrationError = errno != 0 ? errno : EIO;
        observedFd = -1;
        log().error("{}: Descriptor registration failed: fd={}", getName(), fd);
        errno = registrationError;
        return false;
    }

    void DescriptorEventReceiver::disable() {
        if (enabled) {
            enabled = false;
            descriptorEventPublisher.disable(this);
            log().trace("{}: Disabled", getName());
        } else {
            log().warn("{}: Double disable", getName());
        }
    }

    void DescriptorEventReceiver::suspend() {
        if (enabled) {
            if (!suspended) {
                suspended = true;
                descriptorEventPublisher.suspend(this);
            } else {
                log().warn("{}: Double suspend", getName());
            }
        } else {
            log().warn("{}: Suspend while not enabled", getName());
        }
    }

    void DescriptorEventReceiver::resume() {
        if (enabled) {
            if (suspended) {
                suspended = false;
                lastTriggered = utils::Timeval::currentTime();
                descriptorEventPublisher.resume(this);
            } else {
                log().warn("{}: Double resume", getName());
            }
        } else {
            log().warn("{}: Resume while not enabled", getName());
        }
    }

    void DescriptorEventReceiver::setTimeout(const utils::Timeval& timeout) {
        if (timeout == TIMEOUT::DEFAULT) {
            this->maxInactivity = initialTimeout;
        } else {
            this->maxInactivity = timeout;
        }

        triggered(utils::Timeval::currentTime());
    }

    utils::Timeval DescriptorEventReceiver::getTimeout(const utils::Timeval& currentTime) const {
        return maxInactivity > 0 ? currentTime > lastTriggered ? maxInactivity - (currentTime - lastTriggered) : 0 : TIMEOUT::MAX;
    }

    void DescriptorEventReceiver::checkTimeout(const utils::Timeval& currentTime) {
        if (maxInactivity > 0 && currentTime - lastTriggered >= maxInactivity) {
            timeoutEvent();
        }
    }

    void DescriptorEventReceiver::onEvent(const utils::Timeval& currentTime) {
        eventCounter++;
        triggered(currentTime);

        dispatchEvent();
    }

    void DescriptorEventReceiver::notifyShutdown(const ShutdownContext& context) {
        onShutdown(context);
    }

    void DescriptorEventReceiver::onShutdown(const ShutdownContext& context) {
        if (context.reason == ShutdownReason::Signal) {
            signalEvent(context.signal);
        } else {
            disable();
        }
    }

    void DescriptorEventReceiver::triggered(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;
    }

    void DescriptorEventReceiver::setEnabled(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;

        observed();
    }

    void DescriptorEventReceiver::setDisabled() {
        unObserved();
    }

} // namespace core
