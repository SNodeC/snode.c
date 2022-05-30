/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "DescriptorEventReceiver.h"

#include "DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    const utils::Timeval DescriptorEventReceiver::TIMEOUT::DEFAULT = {-2, 0};
    const utils::Timeval DescriptorEventReceiver::TIMEOUT::DISABLE = {-1, 0};
    const utils::Timeval DescriptorEventReceiver::TIMEOUT::MAX = {LONG_MAX, 0};

    DescriptorEventReceiver::DescriptorEventReceiver(const std::string& name,
                                                     DescriptorEventPublisher& descriptorEventPublisher,
                                                     const utils::Timeval& timeout)
        : EventReceiver(name)
        , descriptorEventPublisher(descriptorEventPublisher)
        , maxInactivity(timeout)
        , initialTimeout(timeout) {
    }

    int DescriptorEventReceiver::getRegisteredFd() {
        return observedFd;
    }

    void DescriptorEventReceiver::enable(int fd) {
        if (!enabled) {
            observedFd = fd;

            enabled = true;
            descriptorEventPublisher.enable(this);
        } else {
            LOG(WARNING) << "Double enable: " << getName() << ": fd = " << observedFd;
        }
    }

    void DescriptorEventReceiver::setEnabled(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;

        observed();
    }

    void DescriptorEventReceiver::disable() {
        if (enabled) {
            if (!isSuspended()) {
                suspend();
            }

            enabled = false;
            descriptorEventPublisher.disable(this);
        } else {
            LOG(WARNING) << "Double disable: " << getName() << ": fd = " << observedFd;
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

                if (isObserved()) {
                    descriptorEventPublisher.suspend(this);
                }
            } else {
                LOG(WARNING) << "Double suspend: " << getName() << ": fd = " << observedFd;
            }
        } else {
            LOG(ERROR) << "Suspend while not enabled: " << getName() << ": fd = " << observedFd;
        }
    }

    void DescriptorEventReceiver::resume() {
        if (enabled) {
            if (suspended) {
                suspended = false;
                lastTriggered = utils::Timeval::currentTime();

                if (isObserved()) {
                    descriptorEventPublisher.resume(this);
                }
            } else {
                LOG(WARNING) << "Double resume: " << getName() << ": fd = " << observedFd;
            }
        } else {
            LOG(ERROR) << "Resume while not enabled: " << getName() << ": fd = " << observedFd;
        }
    }

    bool DescriptorEventReceiver::isSuspended() const {
        return suspended;
    }

    void DescriptorEventReceiver::terminate() {
        if (isEnabled()) {
            disable();
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
        return (maxInactivity >= 0) ? maxInactivity - (currentTime - lastTriggered) : TIMEOUT::MAX;
    }

    void DescriptorEventReceiver::event(const utils::Timeval& currentTime) {
        eventCounter++;
        triggered(currentTime);

        dispatchEvent();
    }

    void DescriptorEventReceiver::triggered(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;
    }

    void DescriptorEventReceiver::checkTimeout(const utils::Timeval& currentTime) {
        if (maxInactivity >= 0 && currentTime - lastTriggered >= maxInactivity) {
            timeoutEvent();
        }
    }

} // namespace core
