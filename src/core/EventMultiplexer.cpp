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

#include "EventMultiplexer.h"

#include "DescriptorEventPublisher.h"
#include "DescriptorEventReceiver.h"
#include "DynamicLoader.h"
#include "Event.h" // for Event
#include "TimerEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, max
#include <cerrno>    // for EINTR, errno
#include <numeric>
#include <utility> // for swap

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

    DescriptorEventPublisher& EventMultiplexer::getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE dispType) {
        return *descriptorEventPublishers[dispType];
    }

    core::TimerEventPublisher& EventMultiplexer::getTimerEventPublisher() {
        return *timerEventPublisher;
    }

    void EventMultiplexer::publish(const Event* event) {
        eventQueue.insert(event);
    }

    void EventMultiplexer::unPublish(const Event* event) {
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

    TickStatus EventMultiplexer::tick(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        utils::Timeval currentTime = utils::Timeval::currentTime();

        eventQueue.execute(currentTime);

        unobserveDisabledEvents(currentTime);
        checkTimedOutEvents(currentTime);

        DynamicLoader::execDlCloseDeleyed();

        if (getObservedEventReceiverCount() > 0 || (!timerEventPublisher->empty() && !stopped)) {
            utils::Timeval nextTimeout = 0;

            if (eventQueue.empty()) {
                nextTimeout = getNextTimeout(currentTime);
                nextTimeout = std::min(nextTimeout, tickTimeOut); // In case nextEventTimeout is negativ
            }

            int ret = multiplex(nextTimeout);

            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();
                dispatchActiveEvents(ret, currentTime);
            } else if (errno != EINTR) {
                tickStatus = TickStatus::ERROR;
            } else {
                // ignore EINTR - it is not an error
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    void EventMultiplexer::stop() {
        core::TickStatus tickStatus;

        do {
            for (DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublishers) {
                eventMultiplexer->stop();
            }
            tickStatus = tick(2, true);
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            timerEventPublisher->stop();
            tickStatus = tick(0, false);
        } while (tickStatus == TickStatus::SUCCESS);
    }

    void EventMultiplexer::stopDescriptorEvents() {
        for (DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublishers) {
            eventMultiplexer->stop();
        }
    }

    void EventMultiplexer::stopTimerEvents() {
        timerEventPublisher->stop();
    }

    utils::Timeval EventMultiplexer::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = DescriptorEventReceiver::TIMEOUT::MAX;

        for (DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublishers) {
            nextTimeout = std::min(eventMultiplexer->getNextTimeout(currentTime), nextTimeout);
        }
        nextTimeout = std::min(timerEventPublisher->getNextTimeout(currentTime), nextTimeout);
        nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextTimeout is negative

        return nextTimeout;
    }

    void EventMultiplexer::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
            descriptorEventPublisher->checkTimedOutEvents(currentTime);
        }
    }

    void EventMultiplexer::dispatchActiveEvents(int count, const utils::Timeval& currentTime) {
        timerEventPublisher->dispatchActiveEvents(currentTime);
        dispatchActiveEvents(count);
    }

    void EventMultiplexer::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublishers) {
            eventMultiplexer->unobserveDisabledEvents(currentTime);
        }
        timerEventPublisher->unobsereDisableEvents();
    }

    EventMultiplexer::EventQueue::EventQueue()
        : executeQueue(new std::set<const Event*>()) // cppcheck-suppress [noCopyConstructor, noOperatorEq]
        , publishQueue(new std::set<const Event*>()) {
    }

    EventMultiplexer::EventQueue::~EventQueue() {
        delete executeQueue;
        delete publishQueue;
    }

    void EventMultiplexer::EventQueue::insert(const Event* event) {
        publishQueue->insert(event); // do not allow two or more same events in one tick
    }

    void EventMultiplexer::EventQueue::remove(const Event* event) {
        publishQueue->erase(event); // in case of erase remove the event from the published queue
    }

    void EventMultiplexer::EventQueue::execute(const utils::Timeval& currentTime) {
        std::swap(executeQueue, publishQueue);

        for (const Event* event : *executeQueue) {
            event->dispatch(currentTime);
        }

        executeQueue->clear();
    }

    bool EventMultiplexer::EventQueue::empty() const {
        return publishQueue->empty();
    }

} // namespace core
