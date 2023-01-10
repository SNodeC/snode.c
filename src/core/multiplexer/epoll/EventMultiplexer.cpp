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

#include "core/multiplexer/epoll/EventMultiplexer.h"

#include "core/multiplexer/epoll/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventMultiplexer() {
    static core::epoll::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::epoll {

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(
              new core::epoll::DescriptorEventPublisher("READ", epfds[core::DescriptorEventReceiver::DISP_TYPE::RD], EPOLLIN),
              new core::epoll::DescriptorEventPublisher("WRITE", epfds[core::DescriptorEventReceiver::DISP_TYPE::WR], EPOLLOUT),
              new core::epoll::DescriptorEventPublisher("EXCEPT", epfds[core::DescriptorEventReceiver::DISP_TYPE::EX], EPOLLPRI))
        , epfd(core::system::epoll_create1(EPOLL_CLOEXEC)) {
        epoll_event event{};
        event.events = EPOLLIN;

        event.data.ptr = descriptorEventPublishers[core::DescriptorEventReceiver::DISP_TYPE::RD];
        core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, epfds[core::DescriptorEventReceiver::DISP_TYPE::RD], &event);

        event.data.ptr = descriptorEventPublishers[core::DescriptorEventReceiver::DISP_TYPE::WR];
        core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, epfds[core::DescriptorEventReceiver::DISP_TYPE::WR], &event);

        event.data.ptr = descriptorEventPublishers[core::DescriptorEventReceiver::DISP_TYPE::EX];
        core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, epfds[core::DescriptorEventReceiver::DISP_TYPE::EX], &event);
    }

    int EventMultiplexer::monitorDescriptors(utils::Timeval& tickTimeout) {
        return core::system::epoll_wait(epfd, ePollEvents, 3, tickTimeout.ms());
    }

    void EventMultiplexer::publishActiveEvents() {
        for (int i = 0; i < activeEventCount; i++) {
            if ((ePollEvents[i].events & EPOLLIN) != 0) {
                static_cast<core::DescriptorEventPublisher*>(ePollEvents[i].data.ptr)->publishActiveEvents();
            }
        }
    }

} // namespace core::epoll
