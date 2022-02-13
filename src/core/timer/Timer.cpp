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

#include "core/timer/Timer.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/TimerEventPublisher.h" // for ManagedTimer
#include "core/timer/IntervalTimer.h"
#include "core/timer/SingleshotTimer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    Timer Timer::singleshotTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg) {
        return Timer(new SingleshotTimer(dispatcher, timeout, arg));
    }

    Timer Timer::intervalTimer(const std::function<void(const void*, const std::function<void()>& stop)>& dispatcher,
                               const utils::Timeval& timeout,
                               const void* arg) {
        return Timer(new IntervalTimer(dispatcher, timeout, arg));
    }

    Timer Timer::intervalTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg) {
        return Timer(new IntervalTimer(dispatcher, timeout, arg));
    }

    Timer::Timer(core::TimerEventReceiver* timerEventReceiver)
        : timerEventReceiver(timerEventReceiver) {
        EventLoop::instance().getEventMultiplexer().getTimerEventPublisher().add(timerEventReceiver);
        timerEventReceiver->setTimer(this);
    }

    Timer::Timer(Timer&& timer) {
        VLOG(0) << "Timer moving";

        timerEventReceiver = std::move(timer.timerEventReceiver);
        timerEventReceiver->setTimer(this);
        timer.timerEventReceiver = nullptr;
    }

    Timer::~Timer() {
        if (timerEventReceiver != nullptr) {
            timerEventReceiver->setTimer(nullptr);
        }
    }

    void Timer::cancel() {
        if (timerEventReceiver != nullptr) {
            EventLoop::instance().getEventMultiplexer().getTimerEventPublisher().remove(timerEventReceiver);
        }
    }

    TimerEventReceiver* Timer::getTimerEventReceiver() {
        return timerEventReceiver;
    }

    void Timer::removeTimerEventReceiver() {
        timerEventReceiver = nullptr;
    }

} // namespace core::timer
