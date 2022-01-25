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

#include "core/EventReceiver.h"

#include "core/DescriptorEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    const utils::Timeval EventReceiver::TIMEOUT::DEFAULT = {-2, 0};
    const utils::Timeval EventReceiver::TIMEOUT::DISABLE = {-1, 0};
    const utils::Timeval EventReceiver::TIMEOUT::MAX = {LONG_MAX, 0};

    EventReceiver::EventReceiver(DescriptorEventDispatcher& descriptorEventDispatcher, const utils::Timeval& timeout)
        : descriptorEventDispatcher(descriptorEventDispatcher)
        , maxInactivity(timeout)
        , initialTimeout(timeout) {
    }

    int EventReceiver::getRegisteredFd() {
        return registeredFd;
    }

    void EventReceiver::enable(int fd) {
        this->registeredFd = fd;
        observed();

        descriptorEventDispatcher.enable(this);

        lastTriggered = utils::Timeval::currentTime();

        enabled = true;
    }

    void EventReceiver::disable() {
        if (!isSuspended()) {
            suspend();
        }

        descriptorEventDispatcher.disable(this);
        enabled = false;
    }

    void EventReceiver::disabled() {
        unObserved();
    }

    bool EventReceiver::isEnabled() const {
        return enabled;
    }

    void EventReceiver::suspend() {
        if (isEnabled()) {
            descriptorEventDispatcher.suspend(this);
            suspended = true;
        }
    }

    void EventReceiver::resume() {
        if (isEnabled()) {
            descriptorEventDispatcher.resume(this);
            suspended = false;
            lastTriggered = utils::Timeval::currentTime();
        }
    }

    bool EventReceiver::isSuspended() const {
        return suspended;
    }

    void EventReceiver::terminate() {
        if (isEnabled()) {
            disable();
        }
    }

    void EventReceiver::setTimeout(const utils::Timeval& timeout) {
        if (timeout == TIMEOUT::DEFAULT) {
            this->maxInactivity = initialTimeout;
        } else {
            this->maxInactivity = timeout;
        }

        triggered(utils::Timeval::currentTime());
    }

    utils::Timeval EventReceiver::getTimeout(const utils::Timeval& currentTime) const {
        return (maxInactivity >= 0) ? maxInactivity - (currentTime - lastTriggered) : TIMEOUT::MAX;
    }

    void EventReceiver::dispatch(const utils::Timeval& currentTime) {
        eventCounter++;
        triggered(currentTime);

        dispatchEvent();
    }

    void EventReceiver::triggered(const utils::Timeval& currentTime) {
        lastTriggered = currentTime;
    }

    void EventReceiver::checkTimeout(const utils::Timeval& currentTime) {
        if (maxInactivity >= 0 && currentTime - lastTriggered >= maxInactivity) {
            timeoutEvent();
        }
    }

} // namespace core
