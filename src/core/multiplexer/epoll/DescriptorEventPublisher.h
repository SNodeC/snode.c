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

#ifndef CORE_EPOLL_DESCRIPTOREVENTDISPATCHER_H
#define CORE_EPOLL_DESCRIPTOREVENTDISPATCHER_H

#include "core/DescriptorEventPublisher.h" // IWYU pragma: export

namespace core {
    class DescriptorEventReceiver;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/epoll.h"

#include <cstdint>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    class DescriptorEventPublisher : public core::DescriptorEventPublisher {
    private:
        class EPollEvents {
        public:
            explicit EPollEvents(int& epfd, uint32_t event);

        private:
            void muxMod(int fd, uint32_t events, core::DescriptorEventReceiver* eventReceiver) const;

        public:
            void muxAdd(core::DescriptorEventReceiver* eventReceiver);
            void muxDel(int fd);

            void muxOn(core::DescriptorEventReceiver* eventReceiver);
            void muxOff(core::DescriptorEventReceiver* eventReceiver);

            int getEPFd() const;
            epoll_event* getEvents();
            int getInterestCount() const;

        private:
            int& epfd;
            uint32_t events;

            std::vector<epoll_event> ePollEvents;
            std::vector<epoll_event>::size_type interestCount = 0;
        };

    public:
        DescriptorEventPublisher(const std::string& name, int& epfd, uint32_t events, uint32_t revents);

    private:
        void muxAdd(core::DescriptorEventReceiver* eventReceiver) override;
        void muxDel(int fd) override;
        void muxOn(core::DescriptorEventReceiver* eventReceiver) override;
        void muxOff(core::DescriptorEventReceiver* eventReceiver) override;

        int spanActiveEvents() override;

        EPollEvents ePollEvents;
        uint32_t revents;
    };

} // namespace core::epoll

#endif // CORE_EPOLL_DESCRIPTOREVENTDISPATCHER_H
