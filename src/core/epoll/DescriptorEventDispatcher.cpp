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

#include "core/epoll/DescriptorEventDispatcher.h"

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm> // for find, min
#include <cerrno>
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type, pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    DescriptorEventDispatcher::EPollEvents::EPollEvents(uint32_t events)
        : events(events) {
        epfd = epoll_create1(EPOLL_CLOEXEC);
        interestCount = 0;
        ePollEvents.resize(1);
    }

    void DescriptorEventDispatcher::EPollEvents::add(EventReceiver* eventReceiver) {
        epoll_event ePollEvent;

        ePollEvent.data.ptr = eventReceiver;
        ePollEvent.events = events;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, eventReceiver->getRegisteredFd(), &ePollEvent) == 0) {
            interestCount++;
            if (interestCount >= ePollEvents.size()) {
                ePollEvents.resize(ePollEvents.size() * 2);
            }
        } else {
            switch (errno) {
                case EEXIST:
                    mod(eventReceiver, events);
                    break;
                default:
                    break;
            }
        }
    }

    void DescriptorEventDispatcher::EPollEvents::del(EventReceiver* eventReceiver) {
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, eventReceiver->getRegisteredFd(), nullptr) == 0) {
            interestCount--;
        } else {
            switch (errno) {
                case ENOENT:
                    break;
                default:
                    break;
            }
        }
    }

    void DescriptorEventDispatcher::EPollEvents::mod(EventReceiver* eventReceiver, uint32_t events) {
        epoll_event ePollEvent;

        ePollEvent.data.ptr = eventReceiver;
        ePollEvent.events = events;

        epoll_ctl(epfd, EPOLL_CTL_MOD, eventReceiver->getRegisteredFd(), &ePollEvent);
    }

    void DescriptorEventDispatcher::EPollEvents::modOn(EventReceiver* eventReceiver) {
        mod(eventReceiver, events);
    }

    void DescriptorEventDispatcher::EPollEvents::modOff(EventReceiver* eventReceiver) {
        mod(eventReceiver, 0);
    }

    void DescriptorEventDispatcher::EPollEvents::compress() {
        while (ePollEvents.size() > (interestCount * 2) + 1) {
            ePollEvents.resize(ePollEvents.size() / 2);
        }
    }

    int DescriptorEventDispatcher::EPollEvents::getEPFd() const {
        return epfd;
    }

    epoll_event* DescriptorEventDispatcher::EPollEvents::getEvents() {
        return ePollEvents.data();
    }

    int DescriptorEventDispatcher::EPollEvents::getMaxEvents() const {
        return static_cast<int>(interestCount);
    }
    void DescriptorEventDispatcher::EPollEvents::printStats() {
        VLOG(0) << "EPollEvents stats: Events = " << events << ", size = " << ePollEvents.size() << ", interest count = " << interestCount;
    }

    bool DescriptorEventDispatcher::EventReceiverList::contains(core::EventReceiver* eventReceiver) const {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    DescriptorEventDispatcher::DescriptorEventDispatcher(uint32_t events)
        : ePollEvents(events) {
    }

    void DescriptorEventDispatcher::enable(core::EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (disabledEventReceiver.contains(fd) && disabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as disable
            disabledEventReceiver[fd].remove(eventReceiver);
        } else if (!eventReceiver->isEnabled() &&
                   (!enabledEventReceiver.contains(fd) || !enabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as disable
            enabledEventReceiver[fd].push_back(eventReceiver);
        } else {
            LOG(WARNING) << "EventReceiver double enable " << fd;
        }
    }

    void DescriptorEventDispatcher::disable(core::EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (enabledEventReceiver.contains(fd) && enabledEventReceiver[fd].contains(eventReceiver)) {
            // same tick as enable
            eventReceiver->disabled();
            if (eventReceiver->getObservationCounter() > 0) {
                enabledEventReceiver[fd].remove(eventReceiver);
            }
        } else if (eventReceiver->isEnabled() &&
                   (!disabledEventReceiver.contains(fd) || !disabledEventReceiver[fd].contains(eventReceiver))) {
            // next tick as enable
            disabledEventReceiver[fd].push_back(eventReceiver);
        } else {
            LOG(WARNING) << "EventReceiver double disable " << fd;
        }
    }

    void DescriptorEventDispatcher::suspend(core::EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (!eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                ePollEvents.modOff(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double suspend";
        }
    }

    void DescriptorEventDispatcher::resume(core::EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                ePollEvents.modOn(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double resume " << fd;
        }
    }

    unsigned long DescriptorEventDispatcher::getEventCounter() const {
        return eventCounter;
    }

    int DescriptorEventDispatcher::getReceiverCount() const {
        return static_cast<int>(observedEventReceiver.size());
    }

    int DescriptorEventDispatcher::getEPFd() const {
        return ePollEvents.getEPFd();
    }

    utils::Timeval DescriptorEventDispatcher::getNextTimeout(const utils::Timeval& currentTime) const {
        utils::Timeval nextTimeout = core::EventReceiver::TIMEOUT::MAX;

        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            const core::EventReceiver* eventReceiver = eventReceivers.front();

            if (eventReceiver->isEnabled()) {
                if (!eventReceiver->isSuspended() && eventReceiver->continueImmediately()) {
                    nextTimeout = 0;
                } else {
                    nextTimeout = std::min(eventReceiver->getTimeout(currentTime), nextTimeout);
                }
            } else {
                nextTimeout = 0;
            }
        }

        return nextTimeout;
    }

    void DescriptorEventDispatcher::observeEnabledEvents() {
        for (const auto& [fd, eventReceivers] : enabledEventReceiver) { // cppcheck-suppress unassignedVariable
            for (core::EventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    observedEventReceiver[fd].push_front(eventReceiver);
                    ePollEvents.add(eventReceiver);
                    if (eventReceiver->isSuspended()) {
                        ePollEvents.modOff(eventReceiver);
                    }
                } else {
                    eventReceiver->unobservedEvent();
                }
            }
        }

        enabledEventReceiver.clear();
    }

    void DescriptorEventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime) {
        int count = epoll_wait(ePollEvents.getEPFd(), ePollEvents.getEvents(), ePollEvents.getMaxEvents(), 0);

        for (int i = 0; i < count; i++) {
            core::EventReceiver* eventReceiver = static_cast<core::EventReceiver*>(ePollEvents.getEvents()[i].data.ptr);
            if (!eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                eventCounter++;
                eventReceiver->trigger(currentTime);
            }
        }
    }

    void DescriptorEventDispatcher::dispatchImmediateEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            core::EventReceiver* eventReceiver = eventReceivers.front();
            if (eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                eventCounter++;
                eventReceiver->trigger(currentTime);
            } else if (eventReceiver->isEnabled()) {
                eventReceiver->checkTimeout(currentTime);
            }
        }
    }

    void DescriptorEventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (core::EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    ePollEvents.del(eventReceiver);
                    observedEventReceiver.erase(fd);
                } else if (!observedEventReceiver[fd].front()->isSuspended()) {
                    ePollEvents.modOn(observedEventReceiver[fd].front());
                    observedEventReceiver[fd].front()->triggered(currentTime);
                } else {
                    ePollEvents.modOff(observedEventReceiver[fd].front());
                }
                eventReceiver->disabled();
                if (eventReceiver->getObservationCounter() == 0) {
                    eventReceiver->unobservedEvent();
                }
            }
        }

        ePollEvents.compress();

        disabledEventReceiver.clear();
    }

    void DescriptorEventDispatcher::stop() {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unusedVariable
            for (core::EventReceiver* eventReceiver : eventReceivers) {
                if (eventReceiver->isEnabled()) {
                    eventReceiver->terminate();
                }
            }
        }
    }

} // namespace core::epoll