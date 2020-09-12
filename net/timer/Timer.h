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

#ifndef TIMER_H
#define TIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    class TimerEventDispatcher;

    namespace timer {

        class SingleshotTimer;
        class IntervalTimer;

        class Timer {
        protected:
            Timer(const struct timeval& timeout, const void* arg);

            virtual ~Timer() = default;

        public:
            Timer(const Timer&) = delete;
            Timer& operator=(const Timer& timer) = delete;

            static IntervalTimer& continousTimer(const std::function<void(const void* arg, const std::function<void()>& stop)>& dispatcher,
                                                 const struct timeval& timeout, const void* arg);

            static IntervalTimer& continousTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                                 const void* arg);

            static SingleshotTimer& singleshotTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                                    const void* arg);

            void cancel();

        protected:
            const void* arg;

            void update();
            void destroy();

            virtual bool dispatch() = 0;

            struct timeval& timeout();

            explicit operator struct timeval() const;

        private:
            struct timeval absoluteTimeout {};
            struct timeval delay;

            friend class net::TimerEventDispatcher;
        };

    } // namespace timer

} // namespace net

bool operator<(const struct timeval& tv1, const struct timeval& tv2);
bool operator>(const struct timeval& tv1, const struct timeval& tv2);
bool operator<=(const struct timeval& tv1, const struct timeval& tv2);
bool operator>=(const struct timeval& tv1, const struct timeval& tv2);
bool operator==(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2);
struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);

#endif // TIMER_H
