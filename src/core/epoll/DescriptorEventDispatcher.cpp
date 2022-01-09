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
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for find, min
#include <cerrno>
#include <climits>
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type, pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    DescriptorEventDispatcher::EPollEvents::EPollEvents() {
        epfd = epoll_create1(EPOLL_CLOEXEC);
        size = 0;
        ePollEvents.resize(1);
    }

    void DescriptorEventDispatcher::EPollEvents::add(EventReceiver* eventReceiver, uint32_t events) {
        epoll_event event;

        event.data.ptr = eventReceiver;
        event.events = events;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, eventReceiver->getRegisteredFd(), &event) < 0) {
            switch (errno) {
                case EEXIST:
                    del(eventReceiver);
                    add(eventReceiver, events);
                    break;
                default:
                    break;
            }
        } else {
            size++;
            if (ePollEvents.size() < size) {
                ePollEvents.resize(ePollEvents.size() * 2);
            }
        }
    }

    void DescriptorEventDispatcher::EPollEvents::mod(EventReceiver* eventReceiver, uint32_t events) {
        epoll_event event;

        event.data.ptr = eventReceiver;
        event.events = events;

        epoll_ctl(epfd, EPOLL_CTL_MOD, eventReceiver->getRegisteredFd(), &event);
    }

    void DescriptorEventDispatcher::EPollEvents::del(EventReceiver* eventReceiver) {
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, eventReceiver->getRegisteredFd(), nullptr) < 0) {
            switch (errno) {
                case ENOENT:
                    break;
                default:
                    break;
            }
        } else {
            size--;
            if (ePollEvents.size() > (size / 2) + 1) {
                ePollEvents.resize(ePollEvents.size() / 2);
            }
        }
    }

    int DescriptorEventDispatcher::EPollEvents::getEPFd() const {
        return epfd;
    }

    epoll_event* DescriptorEventDispatcher::EPollEvents::getEvents() {
        return ePollEvents.data();
    }

    int DescriptorEventDispatcher::EPollEvents::getMaxEvents() const {
        return static_cast<int>(size);
    }

    bool DescriptorEventDispatcher::EventReceiverList::contains(core::EventReceiver* eventReceiver) const {
        return std::find(begin(), end(), eventReceiver) != end();
    }

    DescriptorEventDispatcher::DescriptorEventDispatcher(uint32_t events)
        : events(events) {
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
                ePollEvents.del(eventReceiver);
            }
        } else {
            LOG(WARNING) << "EventReceiver double suspend";
        }
    }

    void DescriptorEventDispatcher::resume(core::EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        if (eventReceiver->isSuspended()) {
            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                ePollEvents.add(eventReceiver, events);
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
        //        return ePollEvents.getMaxEvents();
    }

    int DescriptorEventDispatcher::getEPFd() const {
        return ePollEvents.getEPFd();
    }

    utils::Timeval DescriptorEventDispatcher::getNextTimeout(const utils::Timeval& currentTime) const {
        utils::Timeval nextTimeout = {LONG_MAX, 0};

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
                    if (!eventReceiver->isSuspended()) {
                        ePollEvents.add(eventReceiver, events);
                    }
                } else {
                    eventReceiver->unobservedEvent();
                }
            }
        }
        enabledEventReceiver.clear();
    }

    void DescriptorEventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime, const utils::Timeval& timeOut) {
        int count = epoll_wait(ePollEvents.getEPFd(), ePollEvents.getEvents(), ePollEvents.getMaxEvents(), timeOut.ms());

        for (int i = 0; i < count; i++) {
            core::EventReceiver* eventReceiver = static_cast<core::EventReceiver*>(ePollEvents.getEvents()[i].data.ptr);
            if (ePollEvents.getEvents()[i].events != 0 && !eventReceiver->isSuspended() && !eventReceiver->continueImmediately()) {
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
                if (observedEventReceiver[fd].empty() || observedEventReceiver[fd].front()->isSuspended()) {
                    if (observedEventReceiver[fd].empty()) {
                        observedEventReceiver.erase(fd);
                    }
                } else {
                    ePollEvents.add(observedEventReceiver[fd].front(), events);
                    observedEventReceiver[fd].front()->triggered(currentTime);
                }
                eventReceiver->disabled();
                if (eventReceiver->getObservationCounter() == 0) {
                    eventReceiver->unobservedEvent();
                }
            }
        }
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
