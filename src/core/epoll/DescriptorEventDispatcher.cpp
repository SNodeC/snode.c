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

#include "core/epoll/DescriptorEventDispatcher.h"

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    DescriptorEventDispatcher::EPollEvents::EPollEvents(int& epfd, uint32_t events)
        : epfd(epfd)
        , events(events) {
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
        } else if (errno == EEXIST) {
            mod(eventReceiver, events);
        } else {
        }
    }

    void DescriptorEventDispatcher::EPollEvents::del(EventReceiver* eventReceiver) {
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, eventReceiver->getRegisteredFd(), nullptr) == 0) {
            interestCount--;
        } else if (errno == ENOENT) {
        } else {
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

    int DescriptorEventDispatcher::EPollEvents::getInterestCount() const {
        return static_cast<int>(interestCount);
    }

    DescriptorEventDispatcher::DescriptorEventDispatcher(int& epfd, uint32_t events)
        : ePollEvents(epfd, events) {
    }

    void DescriptorEventDispatcher::modAdd(EventReceiver* eventReceiver) {
        ePollEvents.add(eventReceiver);
    }

    void DescriptorEventDispatcher::modDel(EventReceiver* eventReceiver) {
        ePollEvents.del(eventReceiver);
    }

    void DescriptorEventDispatcher::modOn(EventReceiver* eventReceiver) {
        ePollEvents.modOn(eventReceiver);
    }

    void DescriptorEventDispatcher::modOff(EventReceiver* eventReceiver) {
        ePollEvents.modOff(eventReceiver);
    }

    void DescriptorEventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime) {
        int count = epoll_wait(ePollEvents.getEPFd(), ePollEvents.getEvents(), ePollEvents.getInterestCount(), 0);

        for (int i = 0; i < count; i++) {
            core::EventReceiver* eventReceiver = static_cast<core::EventReceiver*>(ePollEvents.getEvents()[i].data.ptr);
            if (!eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                eventCounter++;
                eventReceiver->dispatch(currentTime);
            }
        }
    }

    void core::epoll::DescriptorEventDispatcher::finishTick() {
        ePollEvents.compress();
    }

} // namespace core::epoll
