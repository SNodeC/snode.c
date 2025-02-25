/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include <climits>

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
        , maxInactivity(timeout)
        , initialTimeout(timeout) {
    }

    int DescriptorEventReceiver::getRegisteredFd() const {
        return observedFd;
    }

    bool DescriptorEventReceiver::enable(int fd) {
        if (!enabled) {
            observedFd = fd;

            enabled = true;
            descriptorEventPublisher.enable(this);
            LOG(TRACE) << getName() << ": Enabled";
        } else {
            LOG(WARNING) << getName() << ": Double enable";
        }

        return enabled;
    }

    void DescriptorEventReceiver::setEnabled(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;

        observed();
    }

    void DescriptorEventReceiver::disable() {
        if (enabled) {
            enabled = false;
            descriptorEventPublisher.disable(this);
            LOG(TRACE) << getName() << ": Disabled";
        } else {
            LOG(WARNING) << getName() << ": Double disable";
        }
    }

    void DescriptorEventReceiver::setDisabled() {
        unObserved();
    }

    bool DescriptorEventReceiver::isEnabled() const {
        return enabled;
    }

    void DescriptorEventReceiver::suspend() {
        if (enabled) {
            if (!suspended) {
                suspended = true;
                descriptorEventPublisher.suspend(this);
            } else {
                LOG(WARNING) << getName() << ": Double suspend";
            }
        } else {
            LOG(WARNING) << getName() << ": Suspend while not enabled";
        }
    }

    void DescriptorEventReceiver::resume() {
        if (enabled) {
            if (suspended) {
                suspended = false;
                lastTriggered = utils::Timeval::currentTime();
                descriptorEventPublisher.resume(this);
            } else {
                LOG(WARNING) << getName() << ": Double resume";
            }
        } else {
            LOG(WARNING) << getName() << ": Resume while not enabled";
        }
    }

    bool DescriptorEventReceiver::isSuspended() const {
        return suspended;
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

    void DescriptorEventReceiver::onEvent(const utils::Timeval& currentTime) {
        eventCounter++;
        triggered(currentTime);

        dispatchEvent();
    }

    void DescriptorEventReceiver::onSignal(int signum) {
        signalEvent(signum);
    }

    void DescriptorEventReceiver::triggered(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;
    }

    void DescriptorEventReceiver::checkTimeout(const utils::Timeval& currentTime) {
        if (maxInactivity > 0 && currentTime - lastTriggered >= maxInactivity) {
            timeoutEvent();
        }
    }

} // namespace core
