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

#include "core/EventDispatcher.h"

#include "core/EventReceiver.h"
#include "core/system/time.h"
#include "log/Logger.h"    // for Writer, CWARNING, LOG
#include "utils/Timeval.h" // for operator-, operator<, operator>=

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, find
#include <climits>
#include <iterator>    // for reverse_iterator
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    std::list<EventDispatcher*> EventDispatcher::eventDispatchers;

    EventDispatcher::EventDispatcher() {
        eventDispatchers.push_back(this);
    }

    EventDispatcher::~EventDispatcher() {
        eventDispatchers.remove(this);
    }

    EventDispatcher::FdSet::FdSet() {
        zero();
    }

    void EventDispatcher::FdSet::set(int fd) {
        FD_SET(fd, &registered);
    }

    void EventDispatcher::FdSet::clr(int fd) {
        FD_CLR(fd, &registered);
        FD_CLR(fd, &active);
    }

    int EventDispatcher::FdSet::isSet(int fd) const {
        return FD_ISSET(fd, &active);
    }

    void EventDispatcher::FdSet::zero() {
        FD_ZERO(&registered);
        FD_ZERO(&active);
    }

    fd_set& EventDispatcher::FdSet::get() {
        active = registered;
        return active;
    }

    bool EventDispatcher::DescriptorEventReceiverList::contains(EventReceiver* eventReceiver) const {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    void EventDispatcher::enable(EventReceiver* eventReceiver, int fd) {
        if (disabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as disable
            disabledEventReceiver[fd].remove(eventReceiver);
        } else if (!eventReceiver->isEnabled() &&
                   (!enabledEventReceiver.contains(fd) || !enabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as disable
            enabledEventReceiver[fd].push_back(eventReceiver);
            if (unobservedEventReceiver.contains(eventReceiver)) {
                unobservedEventReceiver.remove(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double enable " << fd;
        }
    }

    void EventDispatcher::disable(EventReceiver* eventReceiver, int fd) {
        if (enabledEventReceiver.contains(fd) && enabledEventReceiver[fd].contains(eventReceiver)) {
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
            LOG(WARNING) << "EventReceiver double disable " << fd;
        }
    }

    void EventDispatcher::suspend(EventReceiver* eventReceiver, int fd) {
        if (!eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.clr(fd);
            }
        } else {
            LOG(WARNING) << "EventReceiver double suspend";
        }
    }

    void EventDispatcher::resume(EventReceiver* eventReceiver, int fd) {
        if (eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.set(fd);
            }
        } else {
            LOG(WARNING) << "EventReceiver double resume " << fd;
        }
    }

    unsigned long EventDispatcher::getEventCounter() const {
        return eventCounter;
    }

    fd_set& EventDispatcher::getFdSet() {
        return fdSet.get();
    }

    int EventDispatcher::getMaxFd() {
        int maxFd = -1;

        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            maxFd = std::max(eventDispatcher->_getMaxFd(), maxFd);
        }

        return maxFd;
    }

    int EventDispatcher::_getMaxFd() const {
        int maxFd = -1;

        if (!observedEventReceiver.empty()) {
            maxFd = observedEventReceiver.rbegin()->first;
        }

        return maxFd;
    }

    timeval EventDispatcher::getNextTimeout() {
        struct timeval currentTime = {core::system::time(nullptr), 0};
        struct timeval nextTimeout = {LONG_MAX, 0};

        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            nextTimeout = std::min(eventDispatcher->_getNextTimeout(currentTime), nextTimeout);
        }

        return nextTimeout;
    }

    timeval EventDispatcher::_getNextTimeout(struct timeval currentTime) const {
        struct timeval nextInactivityTimeout {
            LONG_MAX, 0
        };

        for ([[maybe_unused]] const auto& [fd, eventReceivers] : observedEventReceiver) {
            EventReceiver* eventReceiver = eventReceivers.front();

            if (!eventReceiver->isSuspended()) {
                if (eventReceiver->continueImmediately()) {
                    nextInactivityTimeout = {0, 0};
                } else {
                    struct timeval maxInactivity = eventReceiver->getTimeout();
                    struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                    nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                }
            }
        }

        return nextInactivityTimeout;
    }

    void EventDispatcher::observeEnabledEvents() {
        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_observeEnabledEvents();
        }
    }

    void EventDispatcher::_observeEnabledEvents() {
        for (const auto& [fd, eventReceivers] : enabledEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].push_front(eventReceiver);
                if (!eventReceiver->isSuspended()) {
                    fdSet.set(fd);
                } else {
                    fdSet.clr(fd);
                }
            }
        }
        enabledEventReceiver.clear();
    }

    void EventDispatcher::dispatchActiveEvents() {
        struct timeval currentTime = {core::system::time(nullptr), 0};

        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_dispatchActiveEvents(currentTime);
        }
    }

    void EventDispatcher::_dispatchActiveEvents(struct timeval currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            EventReceiver* eventReceiver = eventReceivers.front();
            struct timeval maxInactivity = eventReceiver->getTimeout();
            if (fdSet.isSet(fd) || (eventReceiver->continueImmediately())) {
                eventCounter++;
                eventReceiver->dispatchEvent();
                eventReceiver->triggered(currentTime);
            } else {
                struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                if (inactivity >= maxInactivity) {
                    eventReceiver->timeoutEvent();
                }
            }
        }
    }

    void EventDispatcher::unobserveDisabledEvents() {
        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_unobserveDisabledEvents();
        }
    }

    void EventDispatcher::_unobserveDisabledEvents() {
        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty() || observedEventReceiver[fd].front()->isSuspended()) {
                    if (observedEventReceiver[fd].empty()) {
                        observedEventReceiver.erase(fd);
                    }
                    fdSet.clr(fd);
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

    void EventDispatcher::releaseUnobservedEvents() {
        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_releaseUnobservedEvents();
        }
    }

    void EventDispatcher::_releaseUnobservedEvents() {
        for (EventReceiver* eventReceiver : unobservedEventReceiver) {
            eventReceiver->unobservedEvent();
        }
        unobservedEventReceiver.clear();
    }

    void EventDispatcher::terminateObservedEvents() {
        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_terminateObservedEvents();
        }
    }

    void EventDispatcher::_terminateObservedEvents() {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                eventReceiver->terminate();
            }
        }
    }

} // namespace core
