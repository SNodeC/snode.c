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

#include "core/EventMultiplexer.h"

#include "core/Event.h" // for Event
#include "core/TimerEventDispatcher.h"
#include "core/eventreceiver/DescriptorEventReceiver.h"
#include "core/multiplexer/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, max
#include <cerrno>    // for EINTR, errno
#include <numeric>
#include <utility> // for swap

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    core::EventMultiplexer::EventMultiplexer(DescriptorEventPublisher* const readDescriptorEventDispatcher,
                                             DescriptorEventPublisher* const writeDescriptorEventDispatcher,
                                             DescriptorEventPublisher* const exceptionDescriptorEventDispatcher)
        : descriptorEventPublisher{readDescriptorEventDispatcher, writeDescriptorEventDispatcher, exceptionDescriptorEventDispatcher}
        , timerEventDispatcher(new core::TimerEventDispatcher()) {
    }

    EventMultiplexer::~EventMultiplexer() {
        for (core::DescriptorEventPublisher* descriptorEventPublisher : descriptorEventPublisher) {
            delete descriptorEventPublisher;
        }

        delete timerEventDispatcher;
    }

    core::DescriptorEventPublisher& EventMultiplexer::getDescriptorEventDispatcher(core::EventMultiplexer::DISP_TYPE dispType) {
        return *descriptorEventPublisher[dispType];
    }

    core::TimerEventDispatcher& EventMultiplexer::getTimerEventDispatcher() {
        return *timerEventDispatcher;
    }

    void EventMultiplexer::publish(const Event* event) {
        eventQueue.insert(event);
    }

    void EventMultiplexer::unPublish(const Event* event) {
        eventQueue.remove(event);
    }

    int EventMultiplexer::getObservedEventReceiverCount() {
        return std::accumulate(descriptorEventPublisher.begin(),
                               descriptorEventPublisher.end(),
                               0,
                               [](int count, core::DescriptorEventPublisher* descriptorEventPublisher) -> int {
                                   return count + descriptorEventPublisher->getObservedEventReceiverCount();
                               });
    }

    int EventMultiplexer::getMaxFd() {
        return std::accumulate(descriptorEventPublisher.begin(),
                               descriptorEventPublisher.end(),
                               -1,
                               [](int count, core::DescriptorEventPublisher* descriptorEventPublisher) -> int {
                                   return std::max(descriptorEventPublisher->getMaxFd(), count);
                               });
    }

    TickStatus EventMultiplexer::tick(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        utils::Timeval currentTime = utils::Timeval::currentTime();

        eventQueue.execute(currentTime);

        checkTimedOutEvents(currentTime);
        unobserveDisabledEvents(currentTime);
        observeEnabledEvents(currentTime);

        if (getObservedEventReceiverCount() > 0 || (!timerEventDispatcher->empty() && !stopped)) {
            utils::Timeval nextTimeout = 0;

            if (eventQueue.empty()) {
                nextTimeout = getNextTimeout(currentTime);

                nextTimeout = std::min(nextTimeout, tickTimeOut);
                nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ
            }

            int ret = multiplex(nextTimeout);

            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();
                dispatchActiveEvents(ret, currentTime);
            } else {
                if (errno != EINTR) {
                    tickStatus = TickStatus::ERROR;
                }
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    void EventMultiplexer::stop() {
        core::TickStatus tickStatus;

        do {
            for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
                eventMultiplexer->stop();
            }
            tickStatus = tick(2, true);
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            timerEventDispatcher->stop();
            tickStatus = tick(0, false);
        } while (tickStatus == TickStatus::SUCCESS);
    }

    void EventMultiplexer::stopDescriptorEvents() {
        for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
            eventMultiplexer->stop();
        }
    }

    void EventMultiplexer::stopTimerEvents() {
        timerEventDispatcher->stop();
    }

    utils::Timeval EventMultiplexer::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = core::eventreceiver::DescriptorEventReceiver::TIMEOUT::MAX;

        for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
            nextTimeout = std::min(eventMultiplexer->getNextTimeout(currentTime), nextTimeout);
        }
        nextTimeout = std::min(timerEventDispatcher->getNextTimeout(currentTime), nextTimeout);

        return nextTimeout;
    }

    void EventMultiplexer::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
            eventMultiplexer->checkTimedOutEvents(currentTime);
        }
    }

    void EventMultiplexer::observeEnabledEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
            eventMultiplexer->observeEnabledEvents(currentTime);
        }
        timerEventDispatcher->observeEnabledEvents();
    }

    void EventMultiplexer::dispatchActiveEvents(int count, utils::Timeval& currentTime) {
        timerEventDispatcher->dispatchActiveEvents(currentTime);
        dispatchActiveEvents(count);
    }

    void EventMultiplexer::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
            eventMultiplexer->unobserveDisabledEvents(currentTime);
        }
        timerEventDispatcher->unobsereDisableEvents();
    }

    EventMultiplexer::EventQueue::EventQueue()
        : executeQueue(new std::set<const Event*>())
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
