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

#include "DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    DescriptorEventPublisher::EPollEvents::EPollEvents(int& epfd, uint32_t events)
        : epfd(epfd)
        , events(events) {
        epfd = core::system::epoll_create1(EPOLL_CLOEXEC);
        interestCount = 0;
        ePollEvents.resize(1);
    }

    void DescriptorEventPublisher::EPollEvents::muxAdd(core::DescriptorEventReceiver* eventReceiver) {
        epoll_event ePollEvent;

        ePollEvent.data.ptr = eventReceiver;
        ePollEvent.events = events;

        if (core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, eventReceiver->getRegisteredFd(), &ePollEvent) == 0) {
            interestCount++;

            if (interestCount >= ePollEvents.size()) {
                ePollEvents.resize(ePollEvents.size() * 2);
            }
        } else if (errno == EEXIST) {
            muxMod(eventReceiver->getRegisteredFd(), events, eventReceiver);
        }
    }

    void DescriptorEventPublisher::EPollEvents::muxDel(int fd) {
        if (core::system::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == 0) {
            interestCount--;
        }

        while (ePollEvents.size() > (interestCount * 2) + 1) {
            ePollEvents.resize(ePollEvents.size() / 2);
        }
    }

    void DescriptorEventPublisher::EPollEvents::muxMod(int fd, uint32_t events, core::DescriptorEventReceiver* eventReceiver) {
        epoll_event ePollEvent;

        ePollEvent.data.ptr = eventReceiver;
        ePollEvent.events = events;

        core::system::epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ePollEvent);
    }

    void DescriptorEventPublisher::EPollEvents::muxOn(core::DescriptorEventReceiver* eventReceiver) {
        muxMod(eventReceiver->getRegisteredFd(), events, eventReceiver);
    }

    void DescriptorEventPublisher::EPollEvents::muxOff(int fd) {
        muxMod(fd, 0, nullptr);
    }

    int DescriptorEventPublisher::EPollEvents::getEPFd() const {
        return epfd;
    }

    epoll_event* DescriptorEventPublisher::EPollEvents::getEvents() {
        return ePollEvents.data();
    }

    int DescriptorEventPublisher::EPollEvents::getInterestCount() const {
        return static_cast<int>(interestCount);
    }

    DescriptorEventPublisher::DescriptorEventPublisher(int& epfd, uint32_t events)
        : ePollEvents(epfd, events) {
    }

    void DescriptorEventPublisher::muxAdd(core::DescriptorEventReceiver* eventReceiver) {
        ePollEvents.muxAdd(eventReceiver);
    }

    void DescriptorEventPublisher::muxDel(int fd) {
        ePollEvents.muxDel(fd);
    }

    void DescriptorEventPublisher::muxOn(core::DescriptorEventReceiver* eventReceiver) {
        ePollEvents.muxOn(eventReceiver);
    }

    void DescriptorEventPublisher::muxOff(int fd) {
        ePollEvents.muxOff(fd);
    }

    void DescriptorEventPublisher::dispatchActiveEvents() {
        int count = core::system::epoll_wait(ePollEvents.getEPFd(), ePollEvents.getEvents(), ePollEvents.getInterestCount(), 0);

        for (int i = 0; i < count; i++) {
            core::DescriptorEventReceiver* eventReceiver = static_cast<core::DescriptorEventReceiver*>(ePollEvents.getEvents()[i].data.ptr);
            if (eventReceiver != nullptr /*&& !eventReceiver->isSuspended()*/) {
                eventCounter++;
                eventReceiver->publish();
            }
        }
    }

} // namespace core::epoll
