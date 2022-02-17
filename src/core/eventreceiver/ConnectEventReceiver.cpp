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

#include "ConnectEventReceiver.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::eventreceiver {

    ConnectEventReceiver::ConnectEventReceiver(const std::string& name, const utils::Timeval& timeout)
        : core::DescriptorEventReceiver(
              "ConnectEventReceiver: " + name,
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

} // namespace core::eventreceiver
