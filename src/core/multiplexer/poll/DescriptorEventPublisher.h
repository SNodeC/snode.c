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

#ifndef CORE_POLL_DESCRIPTOREVENTDISPATCHER_H
#define CORE_POLL_DESCRIPTOREVENTDISPATCHER_H

#include "core/multiplexer/DescriptorEventPublisher.h" // IWYU pragma: export

namespace core {
    namespace eventreceiver {
        class DescriptorEventReceiver;
    }
    namespace poll {
        class PollFds;
    } // namespace poll
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    class DescriptorEventPublisher : public core::DescriptorEventPublisher {
        DescriptorEventPublisher(const DescriptorEventPublisher&) = delete;
        DescriptorEventPublisher& operator=(const DescriptorEventPublisher&) = delete;

    public:
        DescriptorEventPublisher(core::poll::PollFds& pollFds, short events, short revents);

    private:
        void modAdd(core::eventreceiver::DescriptorEventReceiver* eventReceiver) override;
        void modDel(core::eventreceiver::DescriptorEventReceiver* eventReceiver) override;
        void modOn(core::eventreceiver::DescriptorEventReceiver* eventReceiver) override;
        void modOff(core::eventreceiver::DescriptorEventReceiver* eventReceiver) override;

        void dispatchActiveEvents() override;
        void finishTick() override;

        core::poll::PollFds& pollFds;
        short events;
        short revents;
    };

} // namespace core::poll

#endif // CORE_POLL_DESCRIPTOREVENTDISPATCHER_H
