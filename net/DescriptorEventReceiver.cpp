/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "net/DescriptorEventReceiver.h"

#include "net/DescriptorEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {
    DescriptorEventReceiver::DescriptorEventReceiver(DescriptorEventDispatcher& descriptorEventDispatcher, long timeout)
        : descriptorEventDispatcher(descriptorEventDispatcher)
        , maxInactivity(timeout)
        , initialTimeout(timeout) {
    }

    void DescriptorEventReceiver::enable(int fd) {
        this->fd = fd;
        descriptorEventDispatcher.enable(this, fd);
    }

    void DescriptorEventReceiver::disable() {
        descriptorEventDispatcher.disable(this, fd);
    }

    void DescriptorEventReceiver::suspend() {
        descriptorEventDispatcher.suspend(this, fd);
    }

    void DescriptorEventReceiver::resume() {
        descriptorEventDispatcher.resume(this, fd);
    }

    void DescriptorEventReceiver::enabled() {
        ObservationCounter::observationCounter++;
        _enabled = true;
        lastTriggered = {time(nullptr), 0};
    }

    void DescriptorEventReceiver::disabled() {
        this->fd = -1;
        ObservationCounter::observationCounter--;
        _enabled = false;
    }

    bool DescriptorEventReceiver::isEnabled() const {
        return _enabled;
    }

    void DescriptorEventReceiver::suspended() {
        _suspended = true;
    }

    void DescriptorEventReceiver::resumed() {
        _suspended = false;
        lastTriggered = {time(nullptr), 0};
    }

    bool DescriptorEventReceiver::isSuspended() const {
        return _suspended;
    }

    void DescriptorEventReceiver::setTimeout(long timeout) {
        if (timeout != TIMEOUT::DEFAULT) {
            this->maxInactivity = timeout;
        } else {
            this->maxInactivity = initialTimeout;
        }
    }

    timeval DescriptorEventReceiver::getTimeout() const {
        return {maxInactivity, 0};
    }

    void DescriptorEventReceiver::timeoutEvent() {
    }

    void DescriptorEventReceiver::triggered(struct timeval lastTriggered) {
        this->lastTriggered = lastTriggered;
    }

    timeval DescriptorEventReceiver::getLastTriggered() {
        return lastTriggered;
    }

} // namespace net
