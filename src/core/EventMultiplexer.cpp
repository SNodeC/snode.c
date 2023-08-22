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

#include "core/EventMultiplexer.h"

#include "core/DescriptorEventPublisher.h"
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

    core::EventMultiplexer::EventMultiplexer(DescriptorEventPublisher* const readDescriptorEventPublisher,
                                             DescriptorEventPublisher* const writeDescriptorEventPublisher,
                                             DescriptorEventPublisher* const exceptionDescriptorEventPublisher)
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

    int EventMultiplexer::getObservedEventReceiverCount() {
        return std::accumulate(descriptorEventPublishers.begin(),
                               descriptorEventPublishers.end(),
                               0,
                               [](int count, DescriptorEventPublisher* descriptorEventPublisher) -> int {
                                   return count + descriptorEventPublisher->getObservedEventReceiverCount();
                               });
    }

    int EventMultiplexer::getMaxFd() {
        return std::accumulate(descriptorEventPublishers.begin(),
                               descriptorEventPublishers.end(),
                               -1,
                               [](int count, DescriptorEventPublisher* descriptorEventPublisher) -> int {
                                   return std::max(descriptorEventPublisher->getMaxFd(), count);
                               });
    }

    TickStatus EventMultiplexer::tick(const utils::Timeval& tickTimeOut) {
        utils::Timeval currentTime = utils::Timeval::currentTime();

        TickStatus tickStatus = waitForEvents(tickTimeOut, currentTime);

        if (tickStatus == TickStatus::SUCCESS) {
            spanActiveEvents(currentTime);
            executeEventQueue(currentTime);
            checkTimedOutEvents(currentTime);
            releaseExpiredResources(currentTime);
        }

        return tickStatus;
    }

    void EventMultiplexer::exit() {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->exit();
        }
    }

    void EventMultiplexer::stop() {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->stop();
        }
        timerEventPublisher->stop();
    }

    void EventMultiplexer::deletePublishedEvents() {
        eventQueue.deleteEvents();
    }

    void EventMultiplexer::spanActiveEvents(const utils::Timeval& currentTime) {
        timerEventPublisher->spanActiveEvents(currentTime);
        spanActiveEvents();
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

    TickStatus EventMultiplexer::waitForEvents(const utils::Timeval& tickTimeOut, const utils::Timeval& currentTime) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        if (getObservedEventReceiverCount() > 0 || !timerEventPublisher->empty() || !eventQueue.empty()) {
            utils::Timeval nextTimeout = std::min(getNextTimeout(currentTime), tickTimeOut);

            activeEventCount = monitorDescriptors(nextTimeout);

            if (activeEventCount < 0 && errno != EINTR) {
                tickStatus = TickStatus::ERROR;
            } else if (errno == EINTR) {
                tickStatus = TickStatus::INTERRUPTED;
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    utils::Timeval EventMultiplexer::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = DescriptorEventReceiver::TIMEOUT::MAX;

        if (eventQueue.empty()) {
            for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
                nextTimeout = std::min(descriptorEventPublisher->getNextTimeout(currentTime), nextTimeout);
            }
            nextTimeout = std::min(timerEventPublisher->getNextTimeout(currentTime), nextTimeout);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextTimeout is negative
        } else {
            nextTimeout = 0;
        }

        return nextTimeout;
    }

    EventMultiplexer::EventQueue::EventQueue()
        : executeQueue(new std::list<Event*>())
        , publishQueue(new std::list<Event*>()) {
        sigfillset(&newSet);
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

        sigprocmask(SIG_SETMASK, &newSet, &oldSet);

        for (Event* event : *executeQueue) {
            event->dispatch(currentTime);
        }

        sigprocmask(SIG_SETMASK, &oldSet, nullptr);

        executeQueue->clear();
    }

    bool EventMultiplexer::EventQueue::empty() const {
        return publishQueue->empty();
    }

    void EventMultiplexer::EventQueue::deleteEvents() {
        std::swap(executeQueue, publishQueue);

        for (Event* event : *executeQueue) {
            event->getEventReceiver()->destruct();
        }

        executeQueue->clear();
    }

} // namespace core
