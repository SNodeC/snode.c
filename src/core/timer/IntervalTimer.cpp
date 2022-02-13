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

#include "IntervalTimer.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/TimerEventPublisher.h"
#include "core/TimerEventReceiver.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {
    IntervalTimer::IntervalTimer(const std::function<void(const void*, const std::function<void()>&)>& dispatcher,
                                 const utils::Timeval& timeout,
                                 const void* arg)
        : core::TimerEventReceiver(timeout)
        , dispatcherS(dispatcher)
        , arg(arg) {
    }

    IntervalTimer::IntervalTimer(const std::function<void (const void *)> &dispatcher, const utils::Timeval &timeout, const void *arg)
        : core::TimerEventReceiver(timeout)
        , dispatcherC(dispatcher)
        , arg(arg) {
    }

    void IntervalTimer::dispatch(const utils::Timeval &currentTime) {
        if (dispatcherS) {
            LOG(INFO) << "Timer: Dispatch delta = " << (currentTime - getTimeout()).msd() << " ms";
            bool stop = false;
            dispatcherS(arg, [&stop]() -> void {
                stop = true;
            });
            if (stop) {
                cancel();
            } else {
                update();
            }
        } else if (dispatcherC) {
            LOG(INFO) << "Timer: Dispatch delta = " << (currentTime - getTimeout()).msd() << " ms";
            dispatcherC(arg);
            update();
        }
    }

    void IntervalTimer::update() {
        EventLoop::instance().getEventMultiplexer().getTimerEventPublisher().update(this);
    }

    void IntervalTimer::unobservedEvent() {
        dispatcherS = nullptr;
        dispatcherC = nullptr;
        delete this;
    }

} // namespace core::timer
