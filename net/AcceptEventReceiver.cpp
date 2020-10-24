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

#include "AcceptEventReceiver.h"

#include "AcceptEventDispatcher.h"
#include "EventLoop.h"

namespace net {

    AcceptEventReceiver::AcceptEventReceiver()
        : EventReceiver(EventLoop::instance().getAcceptEventDispatcher().getTimeout()) {
    }

    void AcceptEventReceiver::setTimeout(long timeout) {
        EventReceiver::setTimeout(timeout, EventLoop::instance().getAcceptEventDispatcher().getTimeout());
    }

    void AcceptEventReceiver::enable(int fd, long timeout) {
        EventLoop::instance().getAcceptEventDispatcher().enable(this, fd);
        setTimeout(timeout);
    }

    void AcceptEventReceiver::disable(int fd) {
        EventLoop::instance().getAcceptEventDispatcher().disable(this, fd);
    }

    void AcceptEventReceiver::suspend() {
        EventLoop::instance().getAcceptEventDispatcher().suspend(this);
    }

    void AcceptEventReceiver::resume() {
        EventLoop::instance().getAcceptEventDispatcher().resume(this);
    }

} // namespace net
