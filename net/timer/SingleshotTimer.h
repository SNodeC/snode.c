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

#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"

namespace net::timer {

    class SingleshotTimer : public Timer {
    public:
        SingleshotTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg)
            : Timer(timeout, arg)
            , dispatcher(dispatcher) {
        }

        ~SingleshotTimer() override = default;

        bool dispatch() override {
            dispatcher(arg);
            cancel();
            return false;
        }

        SingleshotTimer& operator=(const SingleshotTimer& timer) = delete;

    private:
        std::function<void(const void* arg)> dispatcher;
    };

} // namespace net::timer

#endif // SINGLESHOTTIMER_H
