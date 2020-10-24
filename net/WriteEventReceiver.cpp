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

#include "WriteEventReceiver.h"

#include "EventLoop.h"
#include "WriteEventDispatcher.h"

namespace net {

    WriteEventReceiver::WriteEventReceiver()
        : EventReceiver(EventLoop::instance().getWriteEventDispatcher().getTimeout()) {
    }

    void WriteEventReceiver::setTimeout(long timeout) {
        EventReceiver::setTimeout(timeout, EventLoop::instance().getWriteEventDispatcher().getTimeout());
    }

    void WriteEventReceiver::enable(int fd, long timeout) {
        EventLoop::instance().getWriteEventDispatcher().enable(this, fd);
        setTimeout(timeout);
    }

    void WriteEventReceiver::disable(int fd) {
        EventLoop::instance().getWriteEventDispatcher().disable(this, fd);
    }

    void WriteEventReceiver::suspend(int fd) {
        EventLoop::instance().getWriteEventDispatcher().suspend(this, fd);
    }

    void WriteEventReceiver::resume(int fd) {
        EventLoop::instance().getWriteEventDispatcher().resume(this, fd);
    }

} // namespace net
