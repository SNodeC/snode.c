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

    void DescriptorEventPublisher::enable(DescriptorEventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        VLOG(0) << "Observed: " << eventReceiver->getName() << ", fd = " << fd;
        eventReceiver->setEnabled();
        observedEventReceiver[fd].push_front(eventReceiver);
        muxAdd(eventReceiver);
        if (eventReceiver->isSuspended()) {
            muxOff(fd);
        }
    }

    void DescriptorEventPublisher::disable() {
        observedEventReceiverMapDirty = true;
    }

    void DescriptorEventPublisher::suspend(DescriptorEventReceiver* eventReceiver) {
        muxOff(eventReceiver->getRegisteredFd());
    }

    void DescriptorEventPublisher::resume(DescriptorEventReceiver* eventReceiver) {
        muxOn(eventReceiver);
    }

    void DescriptorEventPublisher::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            eventReceivers.front()->checkTimeout(currentTime);
        }
    }

    void DescriptorEventPublisher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        if (observedEventReceiverMapDirty) {
            std::erase_if(observedEventReceiver, [this, &currentTime](auto& observedEventReceiverEntry) -> bool {
                auto& [fd, observedEventReceiverList] = observedEventReceiverEntry;
                std::erase_if(observedEventReceiverList, [fd](DescriptorEventReceiver* descriptorEventReceiver) -> bool {
                    bool isDisabled = !descriptorEventReceiver->isEnabled();
                    if (isDisabled) {
                        descriptorEventReceiver->setDisabled();
                        if (descriptorEventReceiver->getObservationCounter() == 0) {
                            VLOG(0) << "UnobservedEvent: " << descriptorEventReceiver->getName() << ", fd = " << fd;
                            descriptorEventReceiver->unobservedEvent();
                        }
                    }
                    return isDisabled;
                });
                if (observedEventReceiverList.empty()) {
                    muxDel(fd);
                } else {
                    observedEventReceiver[fd].front()->triggered(currentTime);
                    if (!observedEventReceiver[fd].front()->isSuspended()) {
                        muxOn(observedEventReceiver[fd].front());
                    } else {
                        muxOff(fd);
                    }
                }

                return observedEventReceiverList.empty();
            });

            observedEventReceiverMapDirty = false;
        }
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
