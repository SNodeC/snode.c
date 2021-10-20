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

#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include "net/DescriptorEventDispatcher.h"

#include "log/Logger.h" // for Writer, CWARNING, LOG
#include "net/DescriptorEventReceiver.h"
#include "utils/Timeval.h" // for operator-, operator<, operator>=

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, find
#include <climits>
#include <iterator>    // for reverse_iterator
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    bool DescriptorEventDispatcher::DescriptorEventReceiverList::contains(DescriptorEventReceiver* eventReceiver) const {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    void DescriptorEventDispatcher::enable(DescriptorEventReceiver* eventReceiver, int fd) {
        if (disabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as disable
            disabledEventReceiver[fd].remove(eventReceiver);
        } else if (!eventReceiver->isEnabled() &&
                   (!enabledEventReceiver.contains(fd) || !enabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as disable
            enabledEventReceiver[fd].push_back(eventReceiver);
            eventReceiver->enabled();
            if (unobservedEventReceiver.contains(eventReceiver)) {
                unobservedEventReceiver.remove(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double enable";
        }
    }

    void DescriptorEventDispatcher::disable(DescriptorEventReceiver* eventReceiver, int fd) {
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

    void DescriptorEventDispatcher::suspend(DescriptorEventReceiver* eventReceiver, int fd) {
        if (!eventReceiver->isSuspended()) {
            eventReceiver->suspended();
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.clr(fd, true);
            }
        } else {
            LOG(WARNING) << "EventReceiver double suspend";
        }
    }

    void DescriptorEventDispatcher::resume(DescriptorEventReceiver* eventReceiver, int fd) {
        if (eventReceiver->isSuspended()) {
            eventReceiver->resumed();
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.set(fd);
            }
        } else {
            LOG(WARNING) << "EventReceiver double resume";
        }
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

    fd_set& DescriptorEventDispatcher::getFdSet() {
        return fdSet.get();
    }

    struct timeval DescriptorEventDispatcher::observeEnabledEvents() {
        struct timeval nextTimeout = {LONG_MAX, 0};

        for (const auto& [fd, eventReceivers] : enabledEventReceiver) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
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

        return nextTimeout;
    }

    struct timeval DescriptorEventDispatcher::dispatchActiveEvents(struct timeval currentTime) {
        struct timeval nextInactivityTimeout {
            LONG_MAX, 0
        };

        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            DescriptorEventReceiver* eventReceiver = eventReceivers.front();
            struct timeval maxInactivity = eventReceiver->getTimeout();
            if (fdSet.isSet(fd) || (eventReceiver->continueImmediately())) {
                eventCounter++;
                eventReceiver->dispatchEvent();
                eventReceiver->triggered(currentTime);
                if (!eventReceiver->isSuspended()) {
                    if (eventReceiver->continueImmediately()) {
                        nextInactivityTimeout = {0, 0};
                    } else {
                        nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
                    }
                }
            } else {
                struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                if (inactivity >= maxInactivity) {
                    eventReceiver->timeoutEvent();
                } else {
                    nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                }
            }
        }

        return nextInactivityTimeout;
    }

    void DescriptorEventDispatcher::unobserveDisabledEvents() {
        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
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
        for (DescriptorEventReceiver* eventReceiver : unobservedEventReceiver) {
            eventReceiver->unobserved();
        }
        unobservedEventReceiver.clear();
    }

    void DescriptorEventDispatcher::disableObservedEvents() {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                disabledEventReceiver[fd].push_back(eventReceiver);
            }
        }
    }

} // namespace net

#endif // EVENTDISPATCHER_HPP
