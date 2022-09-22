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

#include "core/eventreceiver/AcceptEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::eventreceiver {

    AcceptEventReceiver::AcceptEventReceiver(const std::string& name, const utils::Timeval& timeout)
        : core::DescriptorEventReceiver("AcceptEventReceiver: " + name, core::DescriptorEventReceiver::DISP_TYPE::RD, timeout) {
    }

    void AcceptEventReceiver::acceptTimeout() {
        disable();
    }

    void AcceptEventReceiver::dispatchEvent() {
        acceptEvent();
    }

    void AcceptEventReceiver::timeoutEvent() {
        acceptTimeout();
    }

    InitAcceptEventReceiver::InitAcceptEventReceiver(const std::string& name)
        : core::EventReceiver("InitAcceptEventReceiver: " + name) {
    }

    void InitAcceptEventReceiver::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
        initAcceptEvent();
    }

} // namespace core::eventreceiver
