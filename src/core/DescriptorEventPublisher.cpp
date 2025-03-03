/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <algorithm>
#include <iterator>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    DescriptorEventPublisher::DescriptorEventPublisher(std::string name)
        : name(std::move(name)) {
    }

    DescriptorEventPublisher::~DescriptorEventPublisher() {
    }

    void DescriptorEventPublisher::enable(DescriptorEventReceiver* descriptorEventReceiver) {
        const int fd = descriptorEventReceiver->getRegisteredFd();

        observedEventReceiverLists[fd].push_front(descriptorEventReceiver);
        muxAdd(descriptorEventReceiver);
        if (descriptorEventReceiver->isSuspended()) {
            muxOff(descriptorEventReceiver);
        }
        descriptorEventReceiver->setEnabled(utils::Timeval::currentTime());
    }

    void DescriptorEventPublisher::disable(DescriptorEventReceiver* descriptorEventReceiver) {
        const int fd = descriptorEventReceiver->getRegisteredFd();

        dirtyEventReceiverLists[&observedEventReceiverLists[fd]].push_back(descriptorEventReceiver);
    }

    void DescriptorEventPublisher::suspend(DescriptorEventReceiver* descriptorEventReceiver) {
        muxOff(descriptorEventReceiver);
    }

    void DescriptorEventPublisher::resume(DescriptorEventReceiver* descriptorEventReceiver) {
        muxOn(descriptorEventReceiver);
    }

    void DescriptorEventPublisher::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (auto& [fd, eventReceivers] : observedEventReceiverLists) {
            eventReceivers.front()->checkTimeout(currentTime);
        }
    }

    void DescriptorEventPublisher::releaseDisabledEvents(const utils::Timeval& currentTime) {
        for (auto& [dirtyDescriptEventReceiverList, disabledDescriptorEventReceivers] : dirtyEventReceiverLists) {
            for (DescriptorEventReceiver* disabledDescriptorEventReceiver : disabledDescriptorEventReceivers) {
                dirtyDescriptEventReceiverList->remove(disabledDescriptorEventReceiver);

                if (dirtyDescriptEventReceiverList->empty()) {
                    const int fd = disabledDescriptorEventReceiver->getRegisteredFd();

                    muxDel(fd);
                    observedEventReceiverLists.erase(fd);
                } else {
                    DescriptorEventReceiver* activeDescriptorEventReceiver = dirtyDescriptEventReceiverList->front();

                    activeDescriptorEventReceiver->triggered(currentTime);
                    if (!activeDescriptorEventReceiver->isSuspended()) {
                        muxOn(activeDescriptorEventReceiver);
                    } else {
                        muxOff(activeDescriptorEventReceiver);
                    }
                }

                disabledDescriptorEventReceiver->setDisabled();
            }
        }

        dirtyEventReceiverLists.clear();
    }

    int DescriptorEventPublisher::getObservedEventReceiverCount() const {
        return static_cast<int>(observedEventReceiverLists.size());
    }

    int DescriptorEventPublisher::maxFd() const {
        int maxFd = -1;

        if (!observedEventReceiverLists.empty()) {
            maxFd = observedEventReceiverLists.rbegin()->first;
        }

        return maxFd;
    }

    utils::Timeval DescriptorEventPublisher::getNextTimeout(const utils::Timeval& currentTime) const {
        utils::Timeval nextTimeout = DescriptorEventReceiver::TIMEOUT::MAX;

        if (dirtyEventReceiverLists.empty()) {
            for (const auto& [fd, eventReceivers] : observedEventReceiverLists) {
                nextTimeout = std::min(eventReceivers.front()->getTimeout(currentTime), nextTimeout);
            }
        } else {
            nextTimeout = 0;
        }

        return nextTimeout;
    }

    void DescriptorEventPublisher::signal(int sigNum) {
        for (auto& [fd, eventReceivers] : observedEventReceiverLists) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                eventReceiver->onSignal(sigNum);
            }
        }
    }

    void DescriptorEventPublisher::disable() {
        for (auto& [fd, eventReceivers] : observedEventReceiverLists) {
            for (DescriptorEventReceiver* eventReceiver : eventReceivers) {
                eventReceiver->disable();
            }
        }
    }

    const std::string& DescriptorEventPublisher::getName() const {
        return name;
    }

} // namespace core
