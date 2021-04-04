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

#ifndef INTERVALTIMER_H
#define INTERVALTIMER_H

#include "net/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::timer {

    class IntervalTimer : public Timer {
    public:
        IntervalTimer(const std::function<void(const void* arg, const std::function<void()>& stop)>& dispatcher,
                      const struct timeval& timeout,
                      const void* arg)
            : Timer(timeout, arg)
            , dispatcherS(dispatcher) {
        }

        IntervalTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg)
            : Timer(timeout, arg)
            , dispatcherC(dispatcher) {
        }

        ~IntervalTimer() override = default;

        IntervalTimer& operator=(const IntervalTimer& timer) = delete;

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
        std::function<void(const void* arg, const std::function<void()>& stop)> dispatcherS = nullptr;
        std::function<void(const void* arg)> dispatcherC = nullptr;
    };

} // namespace net::timer

#endif // INTERVALTIMER_H
