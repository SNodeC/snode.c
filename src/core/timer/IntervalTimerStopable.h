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

#ifndef NET_TIMER_INTERVALTIMERSTOPABLE_H
#define NET_TIMER_INTERVALTIMERSTOPABLE_H

#include "core/TimerEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    class IntervalTimerStopable : public core::TimerEventReceiver {
    public:
        IntervalTimerStopable(const IntervalTimerStopable&) = delete;

        IntervalTimerStopable& operator=(const IntervalTimerStopable& timer) = delete;

    private:
        IntervalTimerStopable(const std::function<void(const std::function<void()>& stop)>& dispatcher,
                              const utils::Timeval& timeout,
                              const std::string& name = "IntervalTimerStopable");

        ~IntervalTimerStopable() override = default;

        void dispatchEvent() final;
        void unobservedEvent() override;

        std::function<void(const std::function<void()>&)> dispatcher = nullptr;

        friend class Timer;
    };

} // namespace core::timer

#endif // NET_TIMER_INTERVALTIMERSTOPABLE_H
