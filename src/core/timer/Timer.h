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

#ifndef NET_TIMER_TIMER_H
#define NET_TIMER_TIMER_H

namespace core {
    class TimerEventReceiver;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    class Timer {
    private:
        explicit Timer(core::TimerEventReceiver* timerEventReceiver);

        Timer() = delete;
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;

    public:
        Timer(Timer&& timer);
        Timer& operator=(Timer&& timer);

        ~Timer();

        static Timer intervalTimer(const std::function<void(const void*, const std::function<void()>& stop)>& dispatcher,
                                   const utils::Timeval& timeout,
                                   const void* arg);

        static Timer intervalTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg);

        static Timer singleshotTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg);

        virtual void cancel();

        core::TimerEventReceiver* getTimerEventReceiver();

        void removeTimerEventReceiver();

    protected:
        core::TimerEventReceiver* timerEventReceiver = nullptr;
    };

} // namespace core::timer

#endif // NET_TIMER_TIMER_H
