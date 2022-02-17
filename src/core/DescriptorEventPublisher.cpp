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

#include "DescriptorEventPublisher.h"

#include "Descriptor.h"
#include "DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <iterator>    // for reverse_iterator
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    bool DescriptorEventPublisher::EventReceiverList::contains(DescriptorEventReceiver* eventReceiver) const {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    void DescriptorEventPublisher::enable(DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        enabledEventReceiver[fd].push_back(eventReceiver);
    }

    void DescriptorEventPublisher::disable(DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        disabledEventReceiver[fd].push_back(eventReceiver);
    }

    void DescriptorEventPublisher::suspend(DescriptorEventReceiver* eventReceiver) {
        if (eventReceiver->isObserved()) {
            modOff(eventReceiver);
        }
    }

    void DescriptorEventPublisher::resume(DescriptorEventReceiver* eventReceiver) {
        if (eventReceiver->isObserved()) {
            modOn(eventReceiver);
        }
    }

    void DescriptorEventPublisher::observeEnabledEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : enabledEventReceiver) { // cppcheck-suppress unassignedVariable
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                VLOG(0) << "Observed: " << eventReceiver->getName() << ", fd = " << fd;
                eventReceiver->setEnabled();
                eventReceiver->triggered(currentTime);
                observedEventReceiver[fd].push_front(eventReceiver);
                modAdd(eventReceiver);
                if (eventReceiver->isSuspended()) {
                    modOff(eventReceiver);
                }
            }
        }
        enabledEventReceiver.clear();
    }

    void DescriptorEventPublisher::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for ([[maybe_unused]] const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            eventReceivers.front()->checkTimeout(currentTime);
        }
    }

    void DescriptorEventPublisher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                VLOG(0) << "Unobserved: " << eventReceiver->getName() << ", fd = " << fd;
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    modDel(eventReceiver);
                    observedEventReceiver.erase(fd);
                } else {
                    observedEventReceiver[fd].front()->triggered(currentTime);
                    if (!observedEventReceiver[fd].front()->isSuspended()) {
                        modOn(observedEventReceiver[fd].front());
                    } else {
                        modOff(observedEventReceiver[fd].front());
                    }
                }
                eventReceiver->setDisabled();
                if (eventReceiver->getObservationCounter() == 0) {
                    VLOG(0) << "UnobservedEvent: " << eventReceiver->getName() << ", fd = " << fd;
                    eventReceiver->unobservedEvent();
                }
            }
        }

        disabledEventReceiver.clear();
    }

    int DescriptorEventPublisher::getObservedEventReceiverCount() const {
        return static_cast<int>(observedEventReceiver.size());
    }

    int DescriptorEventPublisher::getMaxFd() const {
        int maxFd = -1;

        if (!observedEventReceiver.empty()) {
            maxFd = observedEventReceiver.rbegin()->first;
        }

        return maxFd;
    }

    utils::Timeval DescriptorEventPublisher::getNextTimeout(const utils::Timeval& currentTime) const {
        utils::Timeval nextTimeout = DescriptorEventReceiver::TIMEOUT::MAX;

        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            const DescriptorEventReceiver* eventReceiver = eventReceivers.front();

            if (!eventReceiver->isSuspended()) {
                nextTimeout = std::min(eventReceiver->getTimeout(currentTime), nextTimeout);
            }
        }

        return nextTimeout;
    }

    void DescriptorEventPublisher::stop() {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    eventReceiver->terminate();
                }
            }
        }
    }

} // namespace core
