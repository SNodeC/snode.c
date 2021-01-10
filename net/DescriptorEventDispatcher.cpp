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

#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "DescriptorEventDispatcher.h"
#include "EventReceiver.h"

namespace net {
    bool DescriptorEventDispatcher::EventReceiverList::contains(EventReceiver* eventReceiver) {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    DescriptorEventDispatcher::DescriptorEventDispatcher(FdSet& fdSet, long maxInactivity) // NOLINT(google-runtime-references)
        : fdSet(fdSet)
        , maxInactivity(maxInactivity) {
    }

    void DescriptorEventDispatcher::enable(EventReceiver* eventReceiver, int fd) {
        if (disabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as disable
            disabledEventReceiver[fd].remove(eventReceiver);
        } else if (!eventReceiver->isEnabled() &&
                   (!enabledEventReceiver.contains(fd) || !enabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as disable
            enabledEventReceiver[fd].push_back(eventReceiver);
            eventReceiver->enabled(fd);
            if (unobservedEventReceiver.contains(eventReceiver)) {
                unobservedEventReceiver.remove(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double enable";
        }
    }

    void DescriptorEventDispatcher::disable(EventReceiver* eventReceiver, int fd) {
        if (enabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as enable
            enabledEventReceiver[fd].remove(eventReceiver);
            eventReceiver->disabled();
            if (eventReceiver->observationCounter == 0) {
                unobservedEventReceiver.push_back(eventReceiver);
            }
        } else if (eventReceiver->isEnabled() &&
                   (!disabledEventReceiver.contains(fd) || !disabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as enable
            disabledEventReceiver[fd].push_back(eventReceiver);
        } else {
            LOG(WARNING) << "EventReceiver double disable";
        }
    }

    void DescriptorEventDispatcher::suspend(EventReceiver* eventReceiver, int fd) {
        if (!eventReceiver->isSuspended()) {
            eventReceiver->suspended();
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.clr(fd, true);
            }
        } else {
            LOG(WARNING) << "EventReceiver double suspend";
        }
    }

    void DescriptorEventDispatcher::resume(EventReceiver* eventReceiver, int fd) {
        if (eventReceiver->isSuspended()) {
            eventReceiver->resumed();
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.set(fd);
            }
        } else {
            LOG(WARNING) << "EventReceiver double resume";
        }
    }

    long DescriptorEventDispatcher::getTimeout() const {
        return maxInactivity;
    }

    unsigned long DescriptorEventDispatcher::getEventCounter() const {
        return eventCounter;
    }

    int DescriptorEventDispatcher::getMaxFd() const {
        int maxFd = -1;

        if (!observedEventReceiver.empty()) {
            maxFd = observedEventReceiver.rbegin()->first;
        }

        return maxFd;
    }

    struct timeval DescriptorEventDispatcher::observeEnabledEvents() {
        struct timeval nextTimeout = {LONG_MAX, 0};

        for (auto [fd, eventReceivers] : enabledEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].push_front(eventReceiver);
                if (!eventReceiver->isSuspended()) {
                    fdSet.set(fd);
                    nextTimeout = std::min(nextTimeout, eventReceiver->getTimeout());
                } else {
                    fdSet.clr(fd, true);
                }
            }
        }
        enabledEventReceiver.clear();

        fdSet.prepare();

        return nextTimeout;
    }

    struct timeval DescriptorEventDispatcher::dispatchActiveEvents(struct timeval currentTime) {
        struct timeval nextInactivityTimeout {
            LONG_MAX, 0
        };

        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            EventReceiver* eventReceiver = eventReceivers.front();
            struct timeval maxInactivity = eventReceiver->getTimeout();
            if (fdSet.isSet(fd)) {
                eventCounter++;
                eventReceiver->dispatchEvent();
                eventReceiver->triggered(currentTime);
                nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
            } else {
                struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                if (inactivity >= maxInactivity) {
                    eventReceiver->timeout();
                    eventReceiver->disable();
                } else {
                    nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                }
            }
        }

        return nextInactivityTimeout;
    }

    void DescriptorEventDispatcher::unobserveDisabledEvents() {
        for (auto [fd, eventReceivers] : disabledEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty() || observedEventReceiver[fd].front()->isSuspended()) {
                    if (observedEventReceiver[fd].empty()) {
                        observedEventReceiver.erase(fd);
                    }
                    fdSet.clr(fd, true);
                } else {
                    fdSet.set(fd);
                    observedEventReceiver[fd].front()->triggered();
                }
                eventReceiver->disabled();
                if (eventReceiver->observationCounter == 0) {
                    unobservedEventReceiver.push_back(eventReceiver);
                }
                eventReceiver->fd = -1;
            }
        }
        disabledEventReceiver.clear();
    }

    void DescriptorEventDispatcher::releaseUnobservedEvents() {
        for (EventReceiver* eventReceiver : unobservedEventReceiver) {
            eventReceiver->unobserved();
        }
        unobservedEventReceiver.clear();
    }

    void DescriptorEventDispatcher::disableObservedEvents() {
        for (auto& [fd, eventReceivers] : observedEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                disabledEventReceiver[fd].push_back(eventReceiver);
            }
        }
    }
} // namespace net

#endif // EVENTDISPATCHER_HPP
