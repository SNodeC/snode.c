#include "DescriptorEventReceiver.h"

namespace net {

    void DescriptorEventReceiver::enabled(int fd) {
        this->fd = fd;
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

    void DescriptorEventReceiver::timeout() {
    }

    void DescriptorEventReceiver::triggered(timeval _lastTriggered) {
        lastTriggered = _lastTriggered;
    }

    timeval DescriptorEventReceiver::getLastTriggered() {
        return lastTriggered;
    }

} // namespace net
