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

#ifndef CORE_POLL_DESCRIPTOREVENTDISPATCHER_H
#define CORE_POLL_DESCRIPTOREVENTDISPATCHER_H

#include "core/DescriptorEventPublisher.h" // IWYU pragma: export

namespace core {
    class DescriptorEventReceiver;

    namespace poll {
        class PollFdsManager;
    } // namespace poll
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    class DescriptorEventPublisher : public core::DescriptorEventPublisher {
    public:
        DescriptorEventPublisher(const std::string& name, core::poll::PollFdsManager& pollFds, short events, short revents);

    private:
        void muxAdd(core::DescriptorEventReceiver* eventReceiver) override;
        void muxDel(int fd) override;
        void muxOn(core::DescriptorEventReceiver* eventReceiver) override;
        void muxOff(core::DescriptorEventReceiver* eventReceiver) override;

        int spanActiveEvents() override;

        core::poll::PollFdsManager& pollFds;
        short events;
        short revents;
    };

} // namespace core::poll

#endif // CORE_POLL_DESCRIPTOREVENTDISPATCHER_H
