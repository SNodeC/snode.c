/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

// IWYU pragma: no_include <bits/utility.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    DescriptorEventPublisher::DescriptorEventPublisher(std::string name)
        : name(std::move(name)) {
    }

    DescriptorEventPublisher::~DescriptorEventPublisher() {
    }

    void DescriptorEventPublisher::enable(DescriptorEventReceiver* descriptorEventReceiver) {
        int fd = descriptorEventReceiver->getRegisteredFd();
        descriptorEventReceiver->setEnabled(utils::Timeval::currentTime());
        observedEventReceivers[fd].push_front(descriptorEventReceiver);
        muxAdd(descriptorEventReceiver);
        if (descriptorEventReceiver->isSuspended()) {
            muxOff(descriptorEventReceiver);
        }
    }

    void DescriptorEventPublisher::disable([[maybe_unused]] DescriptorEventReceiver* descriptorEventReceiver) {
        observedEventReceiversDirty = true;
    }

    void DescriptorEventPublisher::suspend(DescriptorEventReceiver* descriptorEventReceiver) {
        muxOff(descriptorEventReceiver);
    }

    void DescriptorEventPublisher::resume(DescriptorEventReceiver* descriptorEventReceiver) {
        muxOn(descriptorEventReceiver);
    }

    void DescriptorEventPublisher::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceivers) {
            eventReceivers.front()->checkTimeout(currentTime);
        }
    }

    void DescriptorEventPublisher::releaseDisabledEvents(const utils::Timeval& currentTime) {
        if (observedEventReceiversDirty) {
            observedEventReceiversDirty = false;

            for (auto& [fd, observedEventReceiverList] : observedEventReceivers) {
                DescriptorEventReceiver* beforeFirst = observedEventReceiverList.front();
                if (std::erase_if(observedEventReceiverList, [](DescriptorEventReceiver* descriptorEventReceiver) -> bool {
                        bool isDisabled = !descriptorEventReceiver->isEnabled();
                        if (isDisabled) {
                            descriptorEventReceiver->setDisabled();
                            if (!descriptorEventReceiver->isObserved()) {
                                descriptorEventReceiver->unobservedEvent();
                            }
                        }
                        return isDisabled;
                    }) > 0) {
                    if (observedEventReceiverList.empty()) {
                        muxDel(fd);
                    } else {
                        DescriptorEventReceiver* afterFirst = observedEventReceiverList.front();
                        if (beforeFirst != afterFirst) {
                            afterFirst->triggered(currentTime);
                            if (!afterFirst->isSuspended()) {
                                muxOn(afterFirst);
                            } else {
                                muxOff(afterFirst);
                            }
                        }
                    }
                }
            }

            std::erase_if(observedEventReceivers, [](auto& observedEventReceiversEntry) -> bool {
                return observedEventReceiversEntry.second.empty();
            });
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

        if (!observedEventReceiversDirty) {
            for (const auto& [fd, eventReceivers] : observedEventReceivers) {
                nextTimeout = std::min(eventReceivers.front()->getTimeout(currentTime), nextTimeout);
            }
        } else {
            nextTimeout = 0;
        }

        return nextTimeout;
    }

    void DescriptorEventPublisher::exit() {
        for (const auto& [fd, eventReceivers] : observedEventReceivers) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                eventReceiver->onExit();
            }
        }
    }

    void DescriptorEventPublisher::stop() {
        for (const auto& [fd, eventReceivers] : observedEventReceivers) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                eventReceiver->terminate();
            }
        }
    }

    const std::string& DescriptorEventPublisher::getName() const {
        return name;
    }

} // namespace core
