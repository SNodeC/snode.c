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

#include "core/eventreceiver/ReadEventReceiver.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::eventreceiver {

    ReadEventReceiver::ReadEventReceiver(const std::string& name, const utils::Timeval& timeout)
        : core::DescriptorEventReceiver(
              name + " read",
              core::EventLoop::instance().getEventMultiplexer().getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::RD),
              timeout) {
    }

    void ReadEventReceiver::readTimeout() {
        disable();
    }

    void ReadEventReceiver::dispatchEvent() {
        readEvent();
    }

    void ReadEventReceiver::timeoutEvent() {
        readTimeout();
    }

    void ReadEventReceiver::signalEvent([[maybe_unused]] int signum) {
        disable();
    }

} // namespace core::eventreceiver
