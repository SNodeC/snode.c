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

#include "core/timer/Timer.h"

#include "core/EventLoop.h"
#include "core/TimerEventDispatcher.h" // for ManagedTimer
#include "core/timer/IntervalTimer.h"
#include "core/timer/SingleshotTimer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    Timer::Timer(const struct timeval& timeout, const void* arg)
        : arg(arg)
        , delay(timeout) {
        gettimeofday(&absoluteTimeout, nullptr);
        update();
    }

    SingleshotTimer&
    Timer::singleshotTimer(const std::function<void(const void*)>& dispatcher, const struct timeval& timeout, const void* arg) {
        SingleshotTimer* st = new SingleshotTimer(dispatcher, timeout, arg);

        EventLoop::instance().getTimerEventDispatcher().add(st);

        return *st;
    }

    IntervalTimer& Timer::intervalTimer(const std::function<void(const void*, const std::function<void()>& stop)>& dispatcher,
                                        const struct timeval& timeout,
                                        const void* arg) {
        IntervalTimer* ct = new IntervalTimer(dispatcher, timeout, arg);

        EventLoop::instance().getTimerEventDispatcher().add(ct);

        return *ct;
    }

    IntervalTimer&
    Timer::intervalTimer(const std::function<void(const void*)>& dispatcher, const struct timeval& timeout, const void* arg) {
        IntervalTimer* ct = new IntervalTimer(dispatcher, timeout, arg);

        EventLoop::instance().getTimerEventDispatcher().add(ct);

        return *ct;
    }

    void Timer::cancel() {
        EventLoop::instance().getTimerEventDispatcher().remove(this);
    }

    void Timer::update() {
        absoluteTimeout = absoluteTimeout + delay;
    }

    void Timer::destroy() {
        delete this;
    }

    struct timeval& Timer::timeout() {
        return absoluteTimeout;
    }

    Timer::operator struct timeval() const {
        return absoluteTimeout;
    }

} // namespace core::timer
