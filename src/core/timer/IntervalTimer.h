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

#ifndef NET_TIMER_INTERVALTIMER_H
#define NET_TIMER_INTERVALTIMER_H

#include "core/timer/Timer.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    class IntervalTimer : public Timer {
        IntervalTimer& operator=(const IntervalTimer& timer) = delete;

    public:
        IntervalTimer(const std::function<void(const void*, const std::function<void()>& stop)>& dispatcher,
                      const utils::Timeval& timeout,
                      const void* arg)
            : Timer(timeout, arg)
            , dispatcherS(dispatcher) {
        }

        IntervalTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg)
            : Timer(timeout, arg)
            , dispatcherC(dispatcher) {
        }

        ~IntervalTimer() override = default;

        bool dispatch() override {
            bool stop = false;

            if (dispatcherS) {
                dispatcherS(arg, [&stop]() -> void {
                    stop = true;
                });
                if (stop) {
                    cancel();
                } else {
                    update();
                }
            } else if (dispatcherC) {
                dispatcherC(arg);
                update();
            }

            return !stop;
        }

    private:
        void cancel() override {
            dispatcherS = nullptr;
            dispatcherC = nullptr;
            core::timer::Timer::cancel();
        }

        void unobservedEvent() override {
            core::timer::Timer::unobservedEvent();
        }

        std::function<void(const void*, const std::function<void()>&)> dispatcherS = nullptr;
        std::function<void(const void*)> dispatcherC = nullptr;
    };

} // namespace core::timer

#endif // NET_TIMER_INTERVALTIMER_H
