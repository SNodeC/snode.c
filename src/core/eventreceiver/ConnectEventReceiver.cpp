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

#include "core/eventreceiver/ConnectEventReceiver.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::eventreceiver {

    ConnectEventReceiver::ConnectEventReceiver(const std::string& name, const utils::Timeval& timeout)
        : core::DescriptorEventReceiver(
              name + " connect",
              core::EventLoop::instance().getEventMultiplexer().getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::WR),
              timeout) {
    }

    void ConnectEventReceiver::connectTimeout() {
        disable();
    }

    void ConnectEventReceiver::dispatchEvent() {
        connectEvent();
    }

    void ConnectEventReceiver::timeoutEvent() {
        connectTimeout();
    }

    void ConnectEventReceiver::signalEvent([[maybe_unused]] int signum) {
        disable();
    }

} // namespace core::eventreceiver
