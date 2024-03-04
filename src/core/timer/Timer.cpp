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

#include "core/timer/Timer.h"

#include "core/timer/IntervalTimer.h"
#include "core/timer/IntervalTimerStopable.h"
#include "core/timer/SingleshotTimer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    Timer::~Timer() {
    }

    Timer Timer::singleshotTimer(const std::function<void()>& dispatcher, const utils::Timeval& timeout) {
        return Timer(new SingleshotTimer(dispatcher, timeout));
    }

    Timer Timer::intervalTimer(const std::function<void(const std::function<void()>&)>& dispatcher, const utils::Timeval& timeout) {
        return Timer(new IntervalTimerStopable(dispatcher, timeout));
    }

    Timer Timer::intervalTimer(const std::function<void()>& dispatcher, const utils::Timeval& timeout) {
        return Timer(new IntervalTimer(dispatcher, timeout));
    }

} // namespace core::timer
