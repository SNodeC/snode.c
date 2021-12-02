/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    EventReceiver::EventReceiver(EventDispatcher& descriptorEventDispatcher, long timeout)
        : descriptorEventDispatcher(descriptorEventDispatcher)
        , maxInactivity(timeout)
        , initialTimeout(timeout) {
    }

    void EventReceiver::enable(int fd) {
        this->fd = fd;

        ObservationCounter::observationCounter++;

        descriptorEventDispatcher.enable(this, fd);
        lastTriggered = {time(nullptr), 0};
        _enabled = true;
    }

    void EventReceiver::disable() {
        if (!isSuspended()) {
            suspend();
        }

        descriptorEventDispatcher.disable(this, fd);
        _enabled = false;
    }

    void EventReceiver::disabled() {
        ObservationCounter::observationCounter--;
    }

    bool EventReceiver::isEnabled() const {
        return _enabled;
    }

    void EventReceiver::suspend() {
        if (isEnabled()) {
            descriptorEventDispatcher.suspend(this, fd);
            _suspended = true;
        }
    }

    void EventReceiver::resume() {
        if (isEnabled()) {
            descriptorEventDispatcher.resume(this, fd);
            _suspended = false;
            lastTriggered = {time(nullptr), 0};
        }
    }

    bool EventReceiver::isSuspended() const {
        return _suspended;
    }

    void EventReceiver::terminate() {
        if (isEnabled()) {
            disable();
        }
    }

    void EventReceiver::setTimeout(long timeout) {
        if (timeout == TIMEOUT::DEFAULT) {
            this->maxInactivity = initialTimeout;
        } else {
            this->maxInactivity = timeout;
        }

        triggered();
    }

    timeval EventReceiver::getTimeout() const {
        return {maxInactivity, 0};
    }

    void EventReceiver::triggered(struct timeval lastTriggered) {
        this->lastTriggered = lastTriggered;
    }

    timeval EventReceiver::getLastTriggered() {
        return lastTriggered;
    }
    /*
        void EventReceiver::timeoutEvent() {
            if (isEnabled()) {
                disable();
            }
        }
    */
} // namespace core
