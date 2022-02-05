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

#include "core/DescriptorEventDispatcher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <iterator>    // for reverse_iterator
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    bool DescriptorEventDispatcher::EventReceiverList::contains(core::DescriptorEventReceiver* eventReceiver) const {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    void DescriptorEventDispatcher::publish(Event* event) {
        eventQueue.push_back(event);
    }

    void DescriptorEventDispatcher::executeEventQueue() {
        for (Event* event : eventQueue) {
            event->dispatch();
        }

        eventQueue.clear();
    }

    void DescriptorEventDispatcher::enable(core::DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

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

    void DescriptorEventDispatcher::disable(core::DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (enabledEventReceiver.contains(fd) && enabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as enable
            eventReceiver->disabled();
            if (eventReceiver->getObservationCounter() > 0) {
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

    void DescriptorEventDispatcher::suspend(core::DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (!eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                modOff(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double suspend";
        }
    }

    void DescriptorEventDispatcher::resume(core::DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                modOn(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double resume " << fd;
        }
    }

    void DescriptorEventDispatcher::observeEnabledEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : enabledEventReceiver) { // cppcheck-suppress unassignedVariable
            for (core::DescriptorEventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    eventReceiver->triggered(currentTime);
                    observedEventReceiver[fd].push_front(eventReceiver);
                    modAdd(eventReceiver);
                    if (eventReceiver->isSuspended()) {
                        modOff(eventReceiver);
                    }
                } else {
                    eventReceiver->unobservedEvent();
                }
            }
        }
        enabledEventReceiver.clear();
    }

    void DescriptorEventDispatcher::dispatchImmediateEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
            if (eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                eventCounter++;
                //                eventReceiver->dispatch(currentTime);
                eventReceiver->publish(currentTime);
            } else if (eventReceiver->isEnabled()) {
                eventReceiver->checkTimeout(currentTime);
            }
        }
    }

    void DescriptorEventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (core::DescriptorEventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    modDel(eventReceiver);
                    observedEventReceiver.erase(fd);
                } else if (!observedEventReceiver[fd].front()->isSuspended()) {
                    modOn(observedEventReceiver[fd].front());
                    observedEventReceiver[fd].front()->triggered(currentTime);
                } else {
                    modOff(observedEventReceiver[fd].front());
                }
                eventReceiver->disabled();
                if (eventReceiver->getObservationCounter() == 0) {
                    unobservedEventReceiver[fd].push_back(eventReceiver);
                }
            }
        }

        disabledEventReceiver.clear();

        if (!unobservedEventReceiver.empty()) {
            for (const auto& [fd, eventReceivers] : unobservedEventReceiver) { // cppcheck-suppress unusedVariable
                for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                    eventReceiver->unobservedEvent();
                }
            }
            unobservedEventReceiver.clear();

            finishTick();
        }
    }

    void DescriptorEventDispatcher::finishTick() {
    }

    int DescriptorEventDispatcher::getObservedEventReceiverCount() const {
        return static_cast<int>(observedEventReceiver.size());
    }

    int DescriptorEventDispatcher::getMaxFd() const {
        int maxFd = -1;

        if (!observedEventReceiver.empty()) {
            maxFd = observedEventReceiver.rbegin()->first;
        }

        return maxFd;
    }

    utils::Timeval DescriptorEventDispatcher::getNextTimeout(const utils::Timeval& currentTime) const {
        utils::Timeval nextTimeout = core::DescriptorEventReceiver::TIMEOUT::MAX;

        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            const core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();

            if (eventReceiver->isEnabled()) {
                if (!eventReceiver->isSuspended() && eventReceiver->continueImmediately()) {
                    nextTimeout = 0;
                } else {
                    nextTimeout = std::min(eventReceiver->getTimeout(currentTime), nextTimeout);
                }
            } else {
                nextTimeout = 0;
            }
        }

        return nextTimeout;
    }

    void DescriptorEventDispatcher::stop() {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            for (core::DescriptorEventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    eventReceiver->terminate();
                }
            }
        }
    }

} // namespace core
