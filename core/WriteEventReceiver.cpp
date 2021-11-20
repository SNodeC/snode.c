/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/WriteEventReceiver.h"

#include "core/EventLoop.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    WriteEventReceiver::WriteEventReceiver(long timeout)
        : EventReceiver(EventLoop::instance().getWriteEventDispatcher(), timeout) {
    }

    void WriteEventReceiver::writeTimeout() {
        disable();
    }

    bool WriteEventReceiver::continueWriteImmediately() {
        return false;
    }

    void WriteEventReceiver::dispatchEvent() {
        writeEvent();
    }

    void WriteEventReceiver::timeoutEvent() {
        writeTimeout();
    }

    bool WriteEventReceiver::continueImmediately() {
        return continueWriteImmediately();
    }

} // namespace core
