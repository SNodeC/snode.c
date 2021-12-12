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
        if (disabledEventReceiver.contains(fd) && disabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as disable
            disabledEventReceiver[fd].remove(eventReceiver);
        } else if (!eventReceiver->isEnabled() &&
                   (!enabledEventReceiver.contains(fd) || !enabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as disable
            enabledEventReceiver[fd].push_back(eventReceiver);
        } else {
            LOG(WARNING) << "EventReceiver double enable " << fd;
        }
    }

    void EventDispatcher::disable(EventReceiver* eventReceiver, int fd) {
        if (enabledEventReceiver.contains(fd) && enabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as enable
            eventReceiver->disabled();
            if (eventReceiver->observationCounter > 0) {
                enabledEventReceiver[fd].remove(eventReceiver);
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

    ttime::Timeval EventDispatcher::getNextTimeout() {
        ttime::Timeval currentTime;
        core::system::gettimeofday(currentTime, NULL);

        ttime::Timeval nextTimeout = LONG_MAX;

        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            nextTimeout = std::min(eventDispatcher->_getNextTimeout(currentTime), nextTimeout);
        }

        return nextTimeout;
    }

    ttime::Timeval EventDispatcher::_getNextTimeout(const ttime::Timeval& currentTime) const {
        ttime::Timeval nextInactivityTimeout = LONG_MAX;

        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            EventReceiver* eventReceiver = eventReceivers.front();

            if (!eventReceiver->isSuspended()) {
                if (eventReceiver->continueImmediately()) {
                    nextInactivityTimeout = 0L;
                } else {
                    ttime::Timeval maxInactivity = eventReceiver->getTimeout();
                    ttime::Timeval inactivity = currentTime - eventReceiver->getLastTriggered();
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
                if (eventReceiver->isEnabled()) {
                    observedEventReceiver[fd].push_front(eventReceiver);
                    if (!eventReceiver->isSuspended()) {
                        fdSet.set(fd);
                    } else {
                        fdSet.clr(fd);
                    }
                } else {
                    eventReceiver->unobservedEvent();
                }
            }
        }
        enabledEventReceiver.clear();
    }

    void EventDispatcher::dispatchActiveEvents() {
        //        struct timeval currentTime = {core::system::time(nullptr), 0};
        ttime::Timeval currentTime;
        core::system::gettimeofday(currentTime, nullptr);

        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_dispatchActiveEvents(currentTime);
        }
    }

    void EventDispatcher::_dispatchActiveEvents(const ttime::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            EventReceiver* eventReceiver = eventReceivers.front();
            ttime::Timeval maxInactivity = eventReceiver->getTimeout();
            if (fdSet.isSet(fd) || (eventReceiver->continueImmediately())) {
                eventCounter++;
                eventReceiver->dispatchEvent();
                eventReceiver->triggered();
            } else {
                ttime::Timeval inactivity = currentTime - eventReceiver->getLastTriggered();
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
                    eventReceiver->unobservedEvent();
                }
            }
        }
        disabledEventReceiver.clear();
    }

    void EventDispatcher::terminateObservedEvents() {
        for (EventDispatcher* eventDispatcher : eventDispatchers) {
            eventDispatcher->_terminateObservedEvents();
        }
    }

    void EventDispatcher::_terminateObservedEvents() {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    eventReceiver->terminate();
                }
            }
        }
    }

} // namespace core
