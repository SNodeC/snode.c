/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "OutOfBandEventReceiver.h"

#include "EventLoop.h"
#include "OutOfBandEventDispatcher.h"

namespace net {

    OutOfBandEventReceiver::OutOfBandEventReceiver()
        : EventReceiver(EventLoop::instance().getOutOfBandEventDispatcher().getTimeout()) {
    }

    void OutOfBandEventReceiver::setTimeout(long timeout) {
        EventReceiver::setTimeout(timeout, EventLoop::instance().getOutOfBandEventDispatcher().getTimeout());
    }

    void OutOfBandEventReceiver::enable(int fd, long timeout) {
        EventLoop::instance().getOutOfBandEventDispatcher().enable(this, fd);
        setTimeout(timeout);
    }

    void OutOfBandEventReceiver::disable(int fd) {
        EventLoop::instance().getOutOfBandEventDispatcher().disable(this, fd);
    }

    void OutOfBandEventReceiver::suspend(int fd) {
        EventLoop::instance().getOutOfBandEventDispatcher().suspend(this, fd);
    }

    void OutOfBandEventReceiver::resume(int fd) {
        EventLoop::instance().getOutOfBandEventDispatcher().resume(this, fd);
    }

} // namespace net
