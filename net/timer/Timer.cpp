/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"

#include "EventLoop.h"
#include "IntervalTimer.h"
#include "SingleshotTimer.h"
#include "TimerEventDispatcher.h" // for ManagedTimer

Timer::Timer(const struct timeval& timeout, const void* arg)
    : arg(arg)
    , delay(timeout) {
    gettimeofday(&absoluteTimeout, nullptr);
    update();
}

SingleshotTimer& Timer::singleshotTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                        const void* arg) {
    SingleshotTimer* st = new SingleshotTimer(dispatcher, timeout, arg);

    EventLoop::instance().getTimerEventDispatcher().add(st);

    return *st;
}

IntervalTimer& Timer::continousTimer(const std::function<void(const void* arg, const std::function<void()>& stop)>& dispatcher,
                                     const struct timeval& timeout, const void* arg) {
    IntervalTimer* ct = new IntervalTimer(dispatcher, timeout, arg);

    EventLoop::instance().getTimerEventDispatcher().add(ct);

    return *ct;
}

IntervalTimer& Timer::continousTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                     const void* arg) {
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

bool operator<(const struct timeval& tv1, const struct timeval& tv2) {
    return (tv1.tv_sec < tv2.tv_sec) || ((tv1.tv_sec == tv2.tv_sec) && (tv1.tv_usec < tv2.tv_usec));
}

bool operator>(const struct timeval& tv1, const struct timeval& tv2) {
    return (tv2.tv_sec < tv1.tv_sec) || ((tv2.tv_sec == tv1.tv_sec) && (tv2.tv_usec < tv1.tv_usec));
}

bool operator<=(const struct timeval& tv1, const struct timeval& tv2) {
    return !(tv1 > tv2);
}

bool operator>=(const struct timeval& tv1, const struct timeval& tv2) {
    return !(tv1 < tv2);
}

bool operator==(const struct timeval& tv1, const struct timeval& tv2) {
    return !(tv1 < tv2) && !(tv2 < tv1);
}

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2) {
    struct timeval help {};

    help.tv_sec = tv1.tv_sec + tv2.tv_sec;

    help.tv_usec = tv1.tv_usec + tv2.tv_usec;

    if (help.tv_usec > 999999) {
        help.tv_usec -= 1000000;
        help.tv_sec++;
    }

    return help;
}

struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2) {
    struct timeval help {};

    help.tv_sec = tv1.tv_sec - tv2.tv_sec;
    help.tv_usec = tv1.tv_usec - tv2.tv_usec;

    if (help.tv_usec < 0) {
        help.tv_usec += 1000000;
        help.tv_sec--;
    }

    return help;
}
