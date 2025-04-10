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

#include "core/EventMultiplexer.h"

#include "core/DescriptorEventPublisher.h"
#include "core/DescriptorEventReceiver.h"
#include "core/DynamicLoader.h"
#include "core/Event.h"
#include "core/TimerEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <algorithm>
#include <cerrno>
#include <numeric>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    core::EventMultiplexer::EventMultiplexer(DescriptorEventPublisher* readDescriptorEventPublisher,
                                             DescriptorEventPublisher* writeDescriptorEventPublisher,
                                             DescriptorEventPublisher* exceptionDescriptorEventPublisher)
        : descriptorEventPublishers{readDescriptorEventPublisher, writeDescriptorEventPublisher, exceptionDescriptorEventPublisher}
        , timerEventPublisher(new core::TimerEventPublisher()) {
    }

    EventMultiplexer::~EventMultiplexer() {
        for (DescriptorEventPublisher* descriptorEventPublisher : descriptorEventPublishers) {
            delete descriptorEventPublisher;
        }
        delete timerEventPublisher;
    }

    DescriptorEventPublisher& EventMultiplexer::getDescriptorEventPublisher(DISP_TYPE dispType) {
        return *descriptorEventPublishers[dispType];
    }

    core::TimerEventPublisher& EventMultiplexer::getTimerEventPublisher() {
        return *timerEventPublisher;
    }

    void EventMultiplexer::span(Event* event) {
        eventQueue.insert(event);
    }

    void EventMultiplexer::relax(Event* event) {
        eventQueue.remove(event);
    }

    TickStatus EventMultiplexer::tick(const utils::Timeval& tickTimeOut, const sigset_t& sigMask) {
        const utils::Timeval currentTime = utils::Timeval::currentTime();

        int activeDescriptorCount = 0;

        const TickStatus tickStatus = waitForEvents(tickTimeOut, currentTime, sigMask, activeDescriptorCount);

        if (tickStatus == TickStatus::SUCCESS) {
            spanActiveEvents(currentTime, activeDescriptorCount);
            executeEventQueue(currentTime);
            checkTimedOutEvents(currentTime);
            releaseExpiredResources(currentTime);
        }

        return tickStatus;
    }

    void EventMultiplexer::signal(int sigNum) {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->signal(sigNum);
        }
        timerEventPublisher->stop();

        releaseExpiredResources(utils::Timeval::currentTime());
    }

    void EventMultiplexer::terminate() {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->disable();
        }
        timerEventPublisher->stop();

        releaseExpiredResources(utils::Timeval::currentTime());
    }

    void EventMultiplexer::clearEventQueue() {
        eventQueue.clear();
    }

    TickStatus EventMultiplexer::waitForEvents(const utils::Timeval& tickTimeOut,
                                               const utils::Timeval& currentTime,
                                               const sigset_t& sigMask,
                                               int& activeDescriptorCount) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        if (observedEventReceiverCount() > 0 || !timerEventPublisher->empty() || !eventQueue.empty()) {
            utils::Timeval nextTimeout = std::min(getNextTimeout(currentTime), tickTimeOut);

            activeDescriptorCount = monitorDescriptors(nextTimeout, sigMask);

            if (activeDescriptorCount < 0) {
                if (errno == EINTR) {
                    tickStatus = TickStatus::INTERRUPTED;
                } else {
                    tickStatus = TickStatus::TRACE;
                }
            }
        } else {
            tickStatus = TickStatus::NOOBSERVER;
        }

        return tickStatus;
    }

    void EventMultiplexer::spanActiveEvents(const utils::Timeval& currentTime, int activeDescriptorCount) {
        timerEventPublisher->spanActiveEvents(currentTime);
        spanActiveEvents(activeDescriptorCount);
    }

    void EventMultiplexer::executeEventQueue(const utils::Timeval& currentTime) {
        eventQueue.execute(currentTime);
    }

    void EventMultiplexer::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->checkTimedOutEvents(currentTime);
        }
    }

    void EventMultiplexer::releaseExpiredResources(const utils::Timeval& currentTime) {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->releaseDisabledEvents(currentTime);
        }
        timerEventPublisher->unobserveDisableEvents();
        DynamicLoader::execDlCloseDeleyed();
    }

    utils::Timeval EventMultiplexer::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = DescriptorEventReceiver::TIMEOUT::MAX;

        if (eventQueue.empty()) {
            for (const DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
                nextTimeout = std::min(descriptorEventPublisher->getNextTimeout(currentTime), nextTimeout);
            }
            nextTimeout = std::min(timerEventPublisher->getNextTimeout(currentTime), nextTimeout);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextTimeout is negative
        } else {
            nextTimeout = 0;
        }

        return nextTimeout;
    }

    int EventMultiplexer::observedEventReceiverCount() {
        return std::accumulate(descriptorEventPublishers.begin(),
                               descriptorEventPublishers.end(),
                               0,
                               [](int count, const DescriptorEventPublisher* descriptorEventPublisher) -> int {
                                   return count + descriptorEventPublisher->getObservedEventReceiverCount();
                               });
    }

    int EventMultiplexer::maxFd() {
        return std::accumulate(descriptorEventPublishers.begin(),
                               descriptorEventPublishers.end(),
                               -1,
                               [](int count, const DescriptorEventPublisher* descriptorEventPublisher) -> int {
                                   return std::max(descriptorEventPublisher->maxFd(), count);
                               });
    }

    EventMultiplexer::EventQueue::EventQueue()
        : executeQueue(new std::list<Event*>())
        , publishQueue(new std::list<Event*>()) {
    }

    EventMultiplexer::EventQueue::~EventQueue() {
        delete executeQueue;
        delete publishQueue;
    }

    void EventMultiplexer::EventQueue::insert(Event* event) {
        publishQueue->push_back(event); // do not allow two or more same events in one tick
    }

    void EventMultiplexer::EventQueue::remove(Event* event) {
        publishQueue->remove(event); // in case of erase remove the event from the published queue
    }

    void EventMultiplexer::EventQueue::execute(const utils::Timeval& currentTime) {
        std::swap(executeQueue, publishQueue);

        for (Event* event : *executeQueue) {
            event->dispatch(currentTime);
        }

        executeQueue->clear();
    }

    bool EventMultiplexer::EventQueue::empty() const {
        return publishQueue->empty();
    }

    void EventMultiplexer::EventQueue::clear() {
        std::swap(executeQueue, publishQueue);

        for (const Event* event : *executeQueue) {
            event->getEventReceiver()->destruct();
        }

        executeQueue->clear();
    }

} // namespace core
