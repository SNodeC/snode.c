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

#include "DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <iterator>    // for reverse_iterator
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    void DescriptorEventPublisher::enable(DescriptorEventReceiver* descriptorEventReceiver) {
        int fd = descriptorEventReceiver->getRegisteredFd();
        descriptorEventReceiver->setEnabled();
        observedEventReceivers[fd].push_front(descriptorEventReceiver);
        muxAdd(descriptorEventReceiver);
        if (descriptorEventReceiver->isSuspended()) {
            muxOff(fd);
        }
    }

    void DescriptorEventPublisher::disable([[maybe_unused]] DescriptorEventReceiver* descriptorEventReceiver) {
        observedEventReceiversDirty = true;
    }

    void DescriptorEventPublisher::suspend(DescriptorEventReceiver* descriptorEventReceiver) {
        muxOff(descriptorEventReceiver->getRegisteredFd());
    }

    void DescriptorEventPublisher::resume(DescriptorEventReceiver* descriptorEventReceiver) {
        muxOn(descriptorEventReceiver);
    }

    void DescriptorEventPublisher::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceivers) { // cppcheck-suppress unusedVariable
            eventReceivers.front()->checkTimeout(currentTime);
        }
    }

    void DescriptorEventPublisher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        if (observedEventReceiversDirty) {
            std::erase_if(observedEventReceivers, [this, &currentTime](auto& observedEventReceiversEntry) -> bool {
                auto& [fd, observedEventReceiverList] = observedEventReceiversEntry; // cppcheck-suppress constVariable
                std::erase_if(observedEventReceiverList, [](DescriptorEventReceiver* descriptorEventReceiver) -> bool {
                    bool isDisabled = !descriptorEventReceiver->isEnabled();
                    if (isDisabled) {
                        descriptorEventReceiver->setDisabled();
                        if (!descriptorEventReceiver->isObserved()) {
                            descriptorEventReceiver->unobservedEvent();
                        }
                    }
                    return isDisabled;
                });
                if (observedEventReceiverList.empty()) {
                    muxDel(fd);
                } else {
                    DescriptorEventReceiver* descriptorEventReceiver = observedEventReceivers[fd].front();
                    descriptorEventReceiver->triggered(currentTime);
                    if (!descriptorEventReceiver->isSuspended()) {
                        muxOn(descriptorEventReceiver);
                    } else {
                        muxOff(fd);
                    }
                }
                return observedEventReceiverList.empty();
            });
            observedEventReceiversDirty = false;
        }
    }

    int DescriptorEventPublisher::getObservedEventReceiverCount() const {
        return static_cast<int>(observedEventReceivers.size());
    }

    int DescriptorEventPublisher::getMaxFd() const {
        int maxFd = -1;

        if (!observedEventReceivers.empty()) {
            maxFd = observedEventReceivers.rbegin()->first;
        }

        return maxFd;
    }

    utils::Timeval DescriptorEventPublisher::getNextTimeout(const utils::Timeval& currentTime) const {
        utils::Timeval nextTimeout = DescriptorEventReceiver::TIMEOUT::MAX;

        for (const auto& [fd, eventReceivers] : observedEventReceivers) { // cppcheck-suppress unusedVariable
            const DescriptorEventReceiver* eventReceiver = eventReceivers.front();

            if (!eventReceiver->isSuspended()) {
                nextTimeout = std::min(eventReceiver->getTimeout(currentTime), nextTimeout);
            }
        }

        return nextTimeout;
    }

    void DescriptorEventPublisher::stop() {
        for (const auto& [fd, eventReceivers] : observedEventReceivers) { // cppcheck-suppress unusedVariable
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    eventReceiver->terminate();
                }
            }
        }
    }

} // namespace core
