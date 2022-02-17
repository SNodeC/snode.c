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

#ifndef CORE_EPOLL_DESCRIPTOREVENTDISPATCHER_H
#define CORE_EPOLL_DESCRIPTOREVENTDISPATCHER_H

#include "core/DescriptorEventPublisher.h" // IWYU pragma: export

namespace core {
    class DescriptorEventReceiver;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/epoll.h" // IWYU pragma: export

#include <cstdint>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    class DescriptorEventPublisher : public core::DescriptorEventPublisher {
        DescriptorEventPublisher(const DescriptorEventPublisher&) = delete;
        DescriptorEventPublisher& operator=(const DescriptorEventPublisher&) = delete;

    private:
        class EPollEvents {
        public:
            explicit EPollEvents(int& epfd, uint32_t event);

        private:
            void muxMod(int fd, uint32_t events, core::DescriptorEventReceiver* eventReceiver);

        public:
            void muxAdd(core::DescriptorEventReceiver* eventReceiver);
            void muxDel(int fd);

            void muxOn(core::DescriptorEventReceiver* eventReceiver);
            void muxOff(int fd);

            int getEPFd() const;
            epoll_event* getEvents();
            int getInterestCount() const;

        private:
            int& epfd;
            uint32_t events;

            std::vector<epoll_event> ePollEvents;
            uint32_t interestCount;
        };

    public:
        explicit DescriptorEventPublisher(int& epfd, uint32_t events);

    private:
        void muxAdd(core::DescriptorEventReceiver* eventReceiver) override;
        void muxDel(int fd) override;
        void muxOn(core::DescriptorEventReceiver* eventReceiver) override;
        void muxOff(int fd) override;

        void dispatchActiveEvents() override;

    private:
        EPollEvents ePollEvents;
    };

} // namespace core::epoll

#endif // CORE_EPOLL_DESCRIPTOREVENTDISPATCHER_H
