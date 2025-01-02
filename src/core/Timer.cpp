/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/Timer.h"

#include "core/TimerEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    Timer::Timer(TimerEventReceiver* timerEventReceiver)
        : timerEventReceiver(timerEventReceiver) {
        timerEventReceiver->setTimer(this);
        timerEventReceiver->enable();
    }

    Timer::Timer(Timer&& timer) noexcept
        : timerEventReceiver(timer.timerEventReceiver) {
        timer.timerEventReceiver = nullptr;

        if (timerEventReceiver != nullptr) {
            timerEventReceiver->setTimer(this);
        }
    }

    Timer& Timer::operator=(Timer&& timer) noexcept {
        timerEventReceiver = timer.timerEventReceiver;
        timer.timerEventReceiver = nullptr;

        if (timerEventReceiver != nullptr) {
            timerEventReceiver->setTimer(this);
        }

        return *this;
    }

    Timer::~Timer() {
        if (timerEventReceiver != nullptr) {
            timerEventReceiver->setTimer(nullptr);
        }
    }

    void Timer::cancel() {
        if (timerEventReceiver != nullptr) {
            timerEventReceiver->cancel();
        }
    }

    void Timer::restart() {
        if (timerEventReceiver != nullptr) {
            timerEventReceiver->restart();
        }
    }

    void Timer::removeTimerEventReceiver() {
        timerEventReceiver = nullptr;
    }

} // namespace core
