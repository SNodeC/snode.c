/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/EventDispatcher.h"

#include "core/DescriptorEventDispatcher.h"
#include "core/DescriptorEventReceiver.h"
#include "core/Event.h" // for Event
#include "core/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, max
#include <cerrno>    // for EINTR, errno
#include <numeric>
#include <utility> // for swap

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    core::EventDispatcher::EventDispatcher(DescriptorEventDispatcher* const readDescriptorEventDispatcher,
                                           DescriptorEventDispatcher* const writeDescriptorEventDispatcher,
                                           DescriptorEventDispatcher* const exceptionDescriptorEventDispatcher)
        : descriptorEventDispatcher{readDescriptorEventDispatcher, writeDescriptorEventDispatcher, exceptionDescriptorEventDispatcher}
        , timerEventDispatcher(new core::TimerEventDispatcher()) {
    }

    EventDispatcher::~EventDispatcher() {
        for (core::DescriptorEventDispatcher* descriptorEventDispatcher : descriptorEventDispatcher) {
            delete descriptorEventDispatcher;
        }

        delete timerEventDispatcher;
    }

    core::DescriptorEventDispatcher& EventDispatcher::getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) {
        return *descriptorEventDispatcher[dispType];
    }

    core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return *timerEventDispatcher;
    }

    void EventDispatcher::publish(const Event* event) {
        eventQueue.insert(event);
    }

    void EventDispatcher::unPublish(const Event* event) {
        eventQueue.remove(event);
    }

    int EventDispatcher::getObservedEventReceiverCount() {
        return std::accumulate(descriptorEventDispatcher.begin(),
                               descriptorEventDispatcher.end(),
                               0,
                               [](int count, core::DescriptorEventDispatcher* descriptorEventDispatcher) -> int {
                                   return count + descriptorEventDispatcher->getObservedEventReceiverCount();
                               });
    }

    int EventDispatcher::getMaxFd() {
        return std::accumulate(descriptorEventDispatcher.begin(),
                               descriptorEventDispatcher.end(),
                               -1,
                               [](int count, core::DescriptorEventDispatcher* descriptorEventDispatcher) -> int {
                                   return std::max(descriptorEventDispatcher->getMaxFd(), count);
                               });
    }

    TickStatus EventDispatcher::tick(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        utils::Timeval currentTime = utils::Timeval::currentTime();

        eventQueue.execute(currentTime);

        checkTimedOutEvents(currentTime);
        unobserveDisabledEvents(currentTime);
        observeEnabledEvents(currentTime);

        if (getObservedEventReceiverCount() > 0 || (!timerEventDispatcher->empty() && !stopped)) {
            utils::Timeval nextTimeout = 0;

            if (eventQueue.empty()) {
                utils::Timeval nextEventTimeout = getNextTimeout(currentTime);
                utils::Timeval nextTimerTimeout = timerEventDispatcher->getNextTimeout(currentTime);

                nextTimeout = stopped ? nextEventTimeout : std::min(nextTimerTimeout, nextEventTimeout);

                nextTimeout = std::min(nextTimeout, tickTimeOut);
                nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ
            }

            int ret = multiplex(nextTimeout);

            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher->dispatchActiveEvents(currentTime);
                dispatchActiveEvents(ret);
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

    void EventDispatcher::stop() {
        core::TickStatus tickStatus;

        do {
            for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
                eventDispatcher->stop();
            }
            tickStatus = tick(2, true);
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            timerEventDispatcher->stop();
            tickStatus = tick(0, false);
        } while (tickStatus == TickStatus::SUCCESS);
    }

    void EventDispatcher::stopDescriptorEvents() {
        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->stop();
        }
    }

    void EventDispatcher::stopTimerEvents() {
        timerEventDispatcher->stop();
    }

    utils::Timeval EventDispatcher::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = core::DescriptorEventReceiver::TIMEOUT::MAX;

        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            nextTimeout = std::min(eventDispatcher->getNextTimeout(currentTime), nextTimeout);
        }

        return nextTimeout;
    }

    void EventDispatcher::checkTimedOutEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->checkTimedOutEvents(currentTime);
        }
    }

    void EventDispatcher::observeEnabledEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->observeEnabledEvents(currentTime);
        }
    }

    void EventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->unobserveDisabledEvents(currentTime);
        }
    }

    EventDispatcher::EventQueue::EventQueue()
        : executeQueue(new std::set<const Event*>())
        , publishQueue(new std::set<const Event*>()) {
    }

    EventDispatcher::EventQueue::~EventQueue() {
        delete executeQueue;
        delete publishQueue;
    }

    void EventDispatcher::EventQueue::insert(const Event* event) {
        publishQueue->insert(event); // do not allow two or more same events in one tick
    }

    void EventDispatcher::EventQueue::remove(const Event* event) {
        publishQueue->erase(event); // in case of erase remove the event from the published queue
    }

    void EventDispatcher::EventQueue::execute(const utils::Timeval& currentTime) {
        std::swap(executeQueue, publishQueue);

        for (const Event* event : *executeQueue) {
            event->dispatch(currentTime);
        }

        executeQueue->clear();
    }

    bool EventDispatcher::EventQueue::empty() const {
        return publishQueue->empty();
    }

} // namespace core
