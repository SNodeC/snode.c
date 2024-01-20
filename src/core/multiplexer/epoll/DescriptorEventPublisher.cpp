/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "core/multiplexer/epoll/DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/PreserveErrno.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    DescriptorEventPublisher::EPollEvents::EPollEvents(int& epfd, uint32_t events)
        : epfd(epfd)
        , events(events) {
        epfd = core::system::epoll_create1(EPOLL_CLOEXEC);
        ePollEvents.resize(1);
    }

    void DescriptorEventPublisher::EPollEvents::muxAdd(core::DescriptorEventReceiver* eventReceiver) {
        const utils::PreserveErrno preserveErrno;

        epoll_event ePollEvent{};

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
        const utils::PreserveErrno preserveErrno;

        if (core::system::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == 0 || errno == EBADF) {
            interestCount--;

            if (ePollEvents.size() > (interestCount * 2) + 1) {
                ePollEvents.resize(ePollEvents.size() / 2);
                ePollEvents.shrink_to_fit();
            }
        }
    }

    void DescriptorEventPublisher::EPollEvents::muxMod(int fd, uint32_t events, core::DescriptorEventReceiver* eventReceiver) const {
        const utils::PreserveErrno preserveErrno;

        epoll_event ePollEvent{};

        ePollEvent.data.ptr = eventReceiver;
        ePollEvent.events = events;

        core::system::epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ePollEvent);
    }

    void DescriptorEventPublisher::EPollEvents::muxOn(core::DescriptorEventReceiver* eventReceiver) {
        muxMod(eventReceiver->getRegisteredFd(), events, eventReceiver);
    }

    void DescriptorEventPublisher::EPollEvents::muxOff(DescriptorEventReceiver* eventReceiver) {
        muxMod(eventReceiver->getRegisteredFd(), 0, eventReceiver);
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

    DescriptorEventPublisher::DescriptorEventPublisher(const std::string& name, int& epfd, uint32_t events, uint32_t revents)
        : core::DescriptorEventPublisher(name)
        , ePollEvents(epfd, events)
        , revents(revents) {
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

    void DescriptorEventPublisher::muxOff(core::DescriptorEventReceiver* eventReceiver) {
        ePollEvents.muxOff(eventReceiver);
    }

    void DescriptorEventPublisher::spanActiveEvents() {
        const int count = core::system::epoll_wait(ePollEvents.getEPFd(), ePollEvents.getEvents(), ePollEvents.getInterestCount(), 0);

        for (int i = 0; i < count; i++) {
            const epoll_event& ev = ePollEvents.getEvents()[i];
            core::DescriptorEventReceiver* eventReceiver = static_cast<core::DescriptorEventReceiver*>(ev.data.ptr);
            if (eventReceiver != nullptr && (ev.events & revents) != 0) {
                eventCounter++;
                eventReceiver->span();
            }
        }
    }

} // namespace core::epoll
